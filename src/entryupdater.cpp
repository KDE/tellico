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
#include "document.h"

#include <kdialogbase.h>
#include <klocale.h>
#include <klistview.h>
#include <kiconloader.h>

#include <qvbox.h>
#include <qtimer.h>

namespace {
  static const int ENTRY_UPDATE_GOOD_MATCH = 10;
  static const int ENTRY_UPDATE_PERFECT_MATCH = 20;
  static const int CHECK_COLLECTION_IMAGES_STEP_SIZE = 10;
}

using Tellico::EntryUpdater;

// for each entry, we loop over all available fetchers
// then we loop over all entries
EntryUpdater::EntryUpdater(Data::CollPtr coll_, Data::EntryVec entries_, QObject* parent_)
    : QObject(parent_), m_coll(coll_), m_entriesToUpdate(entries_), m_cancelled(false) {
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
    , m_entriesToUpdate(entries_)
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

EntryUpdater::~EntryUpdater() {
  for(ResultList::Iterator res = m_results.begin(); res != m_results.end(); ++res) {
    delete (*res).first;
  }
}

void EntryUpdater::init() {
  m_fetchIndex = 0;
  m_origEntryCount = m_entriesToUpdate.count();
  QString label;
  if(m_entriesToUpdate.count() == 1) {
    label = i18n("Updating %1...").arg(m_entriesToUpdate.front()->title());
  } else {
    label = i18n("Updating entries...");
  }
  Kernel::self()->beginCommandGroup(i18n("Update Entries"));
  ProgressItem& item = ProgressManager::self()->newProgressItem(this, label, true /*canCancel*/);
  item.setTotalSteps(m_fetchers.count() * m_origEntryCount);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));

  // done if no fetchers available
  if(m_fetchers.isEmpty()) {
    QTimer::singleShot(500, this, SLOT(slotCleanup()));
  } else {
    slotStartNext(); // starts fetching
  }
}

void EntryUpdater::slotStartNext() {
  StatusBar::self()->setStatus(i18n("Updating <b>%1</b>...").arg(m_entriesToUpdate.front()->title()));
  ProgressManager::self()->setProgress(this, m_fetchers.count() * (m_origEntryCount - m_entriesToUpdate.count()) + m_fetchIndex);

  Fetch::Fetcher::Ptr f = m_fetchers[m_fetchIndex];
//  myDebug() << "EntryUpdater::slotDone() - starting " << f->source() << endl;
  f->updateEntry(m_entriesToUpdate.front());
}

void EntryUpdater::slotDone() {
  if(m_cancelled) {
    myLog() << "EntryUpdater::slotDone() - cancelled" << endl;
    QTimer::singleShot(500, this, SLOT(slotCleanup()));
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
    m_entriesToUpdate.remove(m_entriesToUpdate.begin());
    // if there are no more entries, and this is the last fetcher, time to delete
    if(m_entriesToUpdate.isEmpty()) {
      QTimer::singleShot(500, this, SLOT(slotCleanup()));
      return;
    }
  }
  kapp->processEvents();
  // so the entry updater can clean up a bit
  QTimer::singleShot(500, this, SLOT(slotStartNext()));
}

void EntryUpdater::slotResult(Fetch::SearchResult* result_) {
  if(!result_ || m_cancelled) {
    return;
  }

//  myDebug() << "EntryUpdater::slotResult() - " << result_->title << " [" << result_->fetcher->source() << "]" << endl;
  m_results.append(UpdateResult(result_, m_fetchers[m_fetchIndex]->updateOverwrite()));
  Data::EntryPtr e = result_->fetchEntry();
  if(e) {
    m_fetchedEntries.append(e);
    int match = m_coll->sameEntry(m_entriesToUpdate.front(), e);
    if(match > ENTRY_UPDATE_PERFECT_MATCH) {
      result_->fetcher->stop();
    }
  }
  kapp->processEvents();
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
  Data::EntryPtr entry = m_entriesToUpdate.front();
  int best = 0;
  ResultList matches;
  for(ResultList::Iterator res = m_results.begin(); res != m_results.end(); ++res) {
    Data::EntryPtr e = (*res).first->fetchEntry();
    if(!e) {
      continue;
    }
    m_fetchedEntries.append(e);
    int match = m_coll->sameEntry(entry, e);
    if(match) {
//      myDebug() << e->title() << " matches by " << match << endl;
    }
    if(match > best) {
      best = match;
      matches.clear();
      matches.append(*res);
    } else if(match == best && best > 0) {
      matches.append(*res);
    }
  }
  if(best < ENTRY_UPDATE_GOOD_MATCH) {
    if(best > 0) {
      myDebug() << "no good match (score > 10), best match = " << best << " (" << matches.count() << " matches)" << endl;
    }
    return;
  }
//  myDebug() << "best match = " << best << " (" << matches.count() << " matches)" << endl;
  UpdateResult match(0, true);
  if(matches.count() == 1) {
    match = matches.front();
  } else if(matches.count() > 1) {
    match = askUser(matches);
  }
  // askUser() could come back with nil
  if(match.first) {
    mergeCurrent(match.first->fetchEntry(), match.second);
  }
}

Tellico::EntryUpdater::UpdateResult EntryUpdater::askUser(ResultList results) {
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
              .arg(m_entriesToUpdate.front()->field(QString::fromLatin1("title")));
  GUI::RichTextLabel* l = new GUI::RichTextLabel(s, hbox);
  hbox->setStretchFactor(l, 100);

  KListView* view = new KListView(box);
  view->setShowSortIndicator(true);
  view->setAllColumnsShowFocus(true);
  view->setResizeMode(QListView::AllColumns);
  view->setMinimumWidth(640);
  view->addColumn(i18n("Title"));
  view->addColumn(i18n("Description"));
  QMap<KListViewItem*, UpdateResult> map;
  for(ResultList::Iterator res = results.begin(); res != results.end(); ++res) {
    map.insert(new KListViewItem(view, (*res).first->fetchEntry()->title(), (*res).first->desc), *res);
  }

  dlg.setMainWidget(box);
  if(dlg.exec() != QDialog::Accepted) {
    return UpdateResult(0, false);
  }
  KListViewItem* item = static_cast<KListViewItem*>(view->selectedItem());
  if(!item) {
    return UpdateResult(0, false);
  }
  return map[item];
}

void EntryUpdater::mergeCurrent(Data::EntryPtr entry_, bool overWrite_) {
  Data::EntryPtr currEntry = m_entriesToUpdate.front();
  if(entry_) {
    m_matchedEntries.append(entry_);
    Kernel::self()->updateEntry(currEntry, entry_, overWrite_);
    if(m_entriesToUpdate.count() % CHECK_COLLECTION_IMAGES_STEP_SIZE == 1) {
      // I don't want to remove any images in the entries that are getting
      // updated since they'll reference them later and the command isn't
      // executed until the command history group is finished
      // so remove pointers to matched entries
      Data::EntryVec nonUpdatedEntries = m_fetchedEntries;
      for(Data::EntryVecIt match = m_matchedEntries.begin(); match != m_matchedEntries.end(); ++match) {
        nonUpdatedEntries.remove(match);
      }
      Data::Document::self()->removeImagesNotInCollection(nonUpdatedEntries, m_matchedEntries);
    }
  }
}

void EntryUpdater::slotCleanup() {
  StatusBar::self()->clearStatus();
  ProgressManager::self()->setDone(this);
  Kernel::self()->endCommandGroup();
  deleteLater();
}

#include "entryupdater.moc"
