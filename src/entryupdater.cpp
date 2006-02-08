/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entryupdater.h"
#include "entry.h"
#include "collection.h"
#include "tellico_kernel.h"
#include "tellico_debug.h"
#include "progressmanager.h"
#include "statusbar.h"
#include "gui/richtextlabel.h"

#include <klocale.h>
#include <klistview.h>
#include <kiconloader.h>

#include <qvbox.h>

using Tellico::EntryUpdater;

// for each entry, we loop over all available fetchers
// then we loop over all entries
EntryUpdater::EntryUpdater(Data::CollPtr coll_, Data::EntryVec entries_, QObject* parent_)
    : QObject(parent_), m_coll(coll_), m_entries(entries_), m_cancelled(false) {
  // for now, we're assuming all entries are same collection type
  m_fetchers = Fetch::Manager::self()->createUpdateFetchers(m_coll->type());
  for(Fetch::FetcherVec::Iterator it = m_fetchers.begin(); it != m_fetchers.end(); ++it) {
    connect(it.data(), SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)),
            SLOT(slotResult(Tellico::Fetch::SearchResult*)));
    connect(it.data(), SIGNAL(signalDone(Tellico::Fetch::Fetcher::Ptr)),
            SLOT(slotDone()));
  }
  init();
}

EntryUpdater::EntryUpdater(const QString& source_, Data::CollPtr coll_, Data::EntryVec entries_, QObject* parent_)
    : QObject(parent_)
    , m_coll(coll_)
    , m_entries(entries_)
    , m_cancelled(false) {
  // for now, we're assuming all entries are same collection type
  Fetch::Fetcher::Ptr f = Fetch::Manager::self()->createUpdateFetcher(m_coll->type(), source_);
  if(f) {
    m_fetchers.append(f);
    connect(f, SIGNAL(signalResultFound(Tellico::Fetch::SearchResult*)),
            SLOT(slotResult(Tellico::Fetch::SearchResult*)));
    connect(f, SIGNAL(signalDone(Tellico::Fetch::Fetcher::Ptr)),
            SLOT(slotDone()));
  }
  init();
}

void EntryUpdater::init() {
  m_fetchIndex = -1;
  m_origEntryCount = m_entries.count();
  m_results.setAutoDelete(true);
  QString label;
  if(m_entries.count() == 1) {
    label = i18n("Updating %1...").arg(m_entries.front()->title());
  } else {
    label = i18n("Updating entries...");
  }
  ProgressItem& item = ProgressManager::self()->newProgressItem(this, label, true /*canCancel*/);
  item.setTotalSteps(m_fetchers.count() * m_origEntryCount);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  slotDone(); // starts fetching
}

void EntryUpdater::slotResult(Fetch::SearchResult* result_) {
  if(!result_ || m_cancelled) {
    return;
  }

//  myDebug() << "EntryUpdater::slotResult() - " << result_->title << " [" << result_->fetcher->source() << "]" << endl;
  m_results.append(result_);
}

void EntryUpdater::slotDone() {
  if(m_cancelled) {
    myDebug() << "EntryUpdater::slotDone() - cancelled" << endl;
    cleanup();
    return;
  }

  if(!m_results.isEmpty()) {
    handleResults();
  }

  m_results.clear();
  ++m_fetchIndex;
//  myDebug() << "EntryUpdater::slotDone() " << m_fetchIndex << endl;
  if(m_fetchIndex == static_cast<int>(m_fetchers.count())) {
    m_fetchIndex = 0;
    // we've gone through the loop for the first entry in the vector
    // pop it and move on
    m_entries.remove(m_entries.begin());
    // if there are no more entries, and this is the last fetcher, time to delete
    if(m_entries.isEmpty()) {
      cleanup();
      return;
    }
  }
  kapp->processEvents();
  StatusBar::self()->setStatus(i18n("Updating <b>%1</b>...").arg(m_entries.front()->title()));
  ProgressManager::self()->setProgress(this, m_fetchers.count() * (m_origEntryCount - m_entries.count()) + m_fetchIndex);

  Fetch::Fetcher::Ptr f = m_fetchers[m_fetchIndex];
//  myDebug() << "EntryUpdater::slotDone() - starting " << f->source() << endl;
  f->updateEntry(m_entries.front());
}

void EntryUpdater::slotCancel() {
//  myDebug() << "EntryUpdater::slotCancel()" << endl;
  m_cancelled = true;
  Fetch::Fetcher::Ptr f = m_fetchers[m_fetchIndex];
  if(f) {
    f->stop(); // ends up calling slotDone();
  } else {
    slotDone();
  }
}

void EntryUpdater::handleResults() {
  Data::EntryPtr entry = m_entries.front();
  int best = 0;
  ResultList matches;
  for(Fetch::SearchResult* res = m_results.first(); res; res = m_results.next()) {
    int match = m_coll->sameEntry(entry, res->fetchEntry());
    if(match) {
//      myDebug() << res->fetchEntry()->title() << " matches by " << match << endl;
    }
    if(match > best) {
      best = match;
      matches.clear();
      matches.append(res);
    } else if(match == best && best > 0) {
      matches.append(res);
    }
  }
  if(best < 10) {
    if(best > 0) {
      myDebug() << "no good match (score > 10), best match = " << best << " (" << matches.count() << " matches)" << endl;
    }
    return;
  }
//  myDebug() << "best match = " << best << " (" << matches.count() << " matches)" << endl;
  if(matches.count() == 1) {
    mergeCurrent(matches.first()->fetchEntry());
  } else if(matches.count() > 1) {
    mergeCurrent(askUser(matches));
  }
}

Tellico::Data::EntryPtr EntryUpdater::askUser(ResultList results) {
  KDialogBase dlg(Kernel::self()->widget(), "entry updater dialog",
                  true, i18n("Select Match"), KDialogBase::Ok|KDialogBase::Cancel);
  QVBox* box = new QVBox(&dlg);
  box->setSpacing(10);

  QHBox* hbox = new QHBox(box);
  hbox->setSpacing(10);
  QLabel* icon = new QLabel(hbox);
  icon->setPixmap(KGlobal::iconLoader()->loadIcon(QString::fromLatin1("network"), KIcon::Panel, 64));
  QString s = i18n("<qt><b>%1</b> returned multiple results which could match <b>%2</b>, "
                   "the entry currently in the collection. Please select the correct match.</qt>")
              .arg(m_fetchers[m_fetchIndex]->source())
              .arg(m_entries.front()->field(QString::fromLatin1("title")));
  GUI::RichTextLabel* l = new GUI::RichTextLabel(s, hbox);
  hbox->setStretchFactor(l, 100);

  KListView* view = new KListView(box);
  view->setShowSortIndicator(true);
  view->setAllColumnsShowFocus(true);
  view->setResizeMode(QListView::AllColumns);
  view->setMinimumWidth(640);
  view->addColumn(i18n("Title"));
  view->addColumn(i18n("Description"));
  QMap<KListViewItem*, Fetch::SearchResult*> map;
  for(QPtrListIterator<Fetch::SearchResult> it(results); it.current(); ++it) {
    map.insert(new KListViewItem(view, it.current()->fetchEntry()->title(), it.current()->desc), it.current());
  }

  dlg.setMainWidget(box);
  if(dlg.exec() != QDialog::Accepted) {
    return 0;
  }
  KListViewItem* item = static_cast<KListViewItem*>(view->selectedItem());
  if(!item) {
    return 0;
  }
  return map[item]->fetchEntry();
}

void EntryUpdater::mergeCurrent(Data::EntryPtr entry_) {
  Data::EntryPtr currEntry = m_entries.front();
  Data::EntryVec olds, news;
  olds.append(new Data::Entry(*currEntry));
  if(entry_ && Data::Collection::mergeEntry(currEntry, entry_)) {
    news.append(currEntry);
    Kernel::self()->modifyEntries(olds, news);
  }
}

void EntryUpdater::cleanup() {
  StatusBar::self()->clearStatus();
  ProgressManager::self()->setDone(this);
  deleteLater();
}

#include "entryupdater.moc"
