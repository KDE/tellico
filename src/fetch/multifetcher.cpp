/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "multifetcher.h"
#include "fetchmanager.h"
#include "../entrycomparison.h"
#include "../document.h"
#include "../gui/collectiontypecombo.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <KConfigGroup>

#include <QLabel>
#include <QVBoxLayout>

using namespace Tellico;
using Tellico::Fetch::MultiFetcher;

MultiFetcher::MultiFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

MultiFetcher::~MultiFetcher() {
}

QString MultiFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool MultiFetcher::canFetch(int type) const {
  return type == m_collType;
}

bool MultiFetcher::canSearch(FetchKey k) const {
  return k == Title ||
         (k == ISBN && m_collType == Data::Collection::Book);
}

void MultiFetcher::readConfigHook(const KConfigGroup& config_) {
  m_collType = config_.readEntry("CollectionType", -1);
  m_uuids = config_.readEntry("Sources", QStringList());
}

void MultiFetcher::readSources() const {
  if(!m_fetchers.isEmpty()) {
    return;
  }
  foreach(const QString& uuid, m_uuids) {
    Fetcher::Ptr fetcher = Manager::self()->fetcherByUuid(uuid);
    if(fetcher) {
      m_fetchers.append(fetcher);
      connect(fetcher.data(), SIGNAL(signalResultFound(Tellico::Fetch::FetchResult*)),
              SLOT(slotResult(Tellico::Fetch::FetchResult*)));
      connect(fetcher.data(), SIGNAL(signalDone(Tellico::Fetch::Fetcher*)),
              SLOT(slotDone()));
    }
  }
}

void MultiFetcher::search() {
  m_started = true;
  readSources();
  foreach(Fetcher::Ptr fetcher, m_fetchers) {
    fetcher->startSearch(request());
  }
}

void MultiFetcher::continueSearch() {
  m_started = true;
  readSources();
  foreach(Fetcher::Ptr fetcher, m_fetchers) {
    fetcher->continueSearch();
  }
}

void MultiFetcher::stop() {
  if(!m_started) {
    return;
  }
  m_started = false;
  foreach(Fetcher::Ptr fetcher, m_fetchers) {
    fetcher->stop();
  }
  // no need to emit done, since slotDone() will get called
}

void MultiFetcher::slotResult(Tellico::Fetch::FetchResult* result) {
  Data::EntryPtr newEntry = result->fetchEntry();
  // first check if we've already received this entry result from another fetcher
  bool alreadyFound = false;
  foreach(Data::EntryPtr entry, m_entries) {
    if(entry->collection()->sameEntry(entry, newEntry) > EntryComparison::ENTRY_GOOD_MATCH) {
      // same entry, so instead of adding a new result, just merge it
      Data::Document::mergeEntry(entry, newEntry);
      alreadyFound = true;
      break;
    }
  }

  if(!alreadyFound) {
    m_entries.append(newEntry);
  }
}

void MultiFetcher::slotDone() {
  //keep it simple, if a little slow
  // iterate over all fetchers, if they are still running, do not emit done
  foreach(Fetcher::Ptr fetcher, m_fetchers) {
    if(fetcher->isSearching())  {
      return;
    }
  }
  // done so emit all results
  foreach(Data::EntryPtr entry, m_entries) {
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entryHash.insert(r->uid, entry);
    emit signalResultFound(r);
  }
  emit signalDone(this);
}

Tellico::Data::EntryPtr MultiFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entryHash[uid_];
  if(!entry) {
    myWarning() << "no entry in hash";
    return Data::EntryPtr();
  }
  return entry;
}

Tellico::Fetch::FetchRequest MultiFetcher::updateRequest(Data::EntryPtr entry_) {
//  myDebug();
  QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* MultiFetcher::configWidget(QWidget* parent_) const {
  return new MultiFetcher::ConfigWidget(parent_, this);
}

QString MultiFetcher::defaultName() {
  return i18n("Multiple Sources");
}

QString MultiFetcher::defaultIcon() {
  return QLatin1String("folder-favorites");
}

Tellico::StringHash MultiFetcher::allOptionalFields() {
  StringHash hash;
  return hash;
}

MultiFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MultiFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());

  KHBox* hbox = new KHBox(optionsWidget());
  l->addWidget(hbox);

  QLabel* label = new QLabel(i18n("Collection &type:"), hbox);
  m_collCombo = new GUI::CollectionTypeCombo(hbox);
  connect(m_collCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  connect(m_collCombo, SIGNAL(activated(int)), SLOT(slotTypeChanged()));
  label->setBuddy(m_collCombo);

  m_listWidget = new FetcherListWidget(optionsWidget());
  l->addWidget(m_listWidget);

  if(fetcher_) {
    if(fetcher_->m_collType > -1) {
      m_collCombo->setCurrentType(fetcher_->m_collType);
    } else {
      m_collCombo->setCurrentType(fetcher_->collectionType());
    }
  } else {
    // default to Book for now
    m_collCombo->setCurrentType(Data::Collection::Book);
  }
  slotTypeChanged();
  // set the source after the type was changed so the fetcher list is already populated
  if(fetcher_) {
    fetcher_->readSources();
    m_listWidget->setSources(fetcher_->m_fetchers);
  }
}

void MultiFetcher::ConfigWidget::slotTypeChanged() {
  const int collType = m_collCombo->currentType();
  m_listWidget->setFetchers(Fetch::Manager::self()->fetchers(collType));
}

void MultiFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("CollectionType", m_collCombo->currentType());
  config_.writeEntry("Sources", m_listWidget->uuids());
}

QString MultiFetcher::ConfigWidget::preferredName() const {
  return MultiFetcher::defaultName();
}

MultiFetcher::FetcherItemWidget::FetcherItemWidget(QWidget* parent_)
    : KHBox(parent_) {
  QLabel* label = new QLabel(QLatin1String("Data source:"), this);
  m_fetcherCombo = new GUI::ComboBox(this);
  connect(m_fetcherCombo, SIGNAL(activated(int)), SIGNAL(signalModified()));
  label->setBuddy(m_fetcherCombo);
}

void MultiFetcher::FetcherItemWidget::setFetchers(const QList<Fetcher::Ptr>& fetchers_) {
  m_fetcherCombo->clear();
  m_fetcherCombo->addItem(QString(), QVariant());
  foreach(Fetcher::Ptr fetcher, fetchers_) {
    // can't contain another multiple fetcher
    if(fetcher->type() == Multiple) {
      continue;
    }
    m_fetcherCombo->addItem(Manager::self()->fetcherIcon(fetcher), fetcher->source(), fetcher->uuid());
  }
}

void MultiFetcher::FetcherItemWidget::setSource(Fetcher::Ptr fetcher_) {
  m_fetcherCombo->setCurrentData(fetcher_->uuid());
}

QString MultiFetcher::FetcherItemWidget::fetcherUuid() const {
  return m_fetcherCombo->currentData().toString();
}

MultiFetcher::FetcherListWidget::FetcherListWidget(QWidget* parent_)
    : KWidgetLister(1, 8, parent_) {
  connect(this, SIGNAL(clearWidgets()), SIGNAL(signalModified()));
  // start with at least 1
  setNumberOfShownWidgetsTo(1);
}

void MultiFetcher::FetcherListWidget::setFetchers(const QList<Fetcher::Ptr>& fetchers_) {
  m_fetchers = fetchers_;
  foreach(QWidget* widget, mWidgetList) {
    FetcherItemWidget* item = static_cast<FetcherItemWidget*>(widget);
    item->setFetchers(fetchers_);
  }
}

void MultiFetcher::FetcherListWidget::setSources(const QList<Fetcher::Ptr>& fetchers_) {
  setNumberOfShownWidgetsTo(fetchers_.count());
  Q_ASSERT(fetchers_.count() == mWidgetList.count());
  int i = 0;
  foreach(QWidget* widget, mWidgetList) {
    FetcherItemWidget* item = static_cast<FetcherItemWidget*>(widget);
    item->setSource(fetchers_.at(i));
    ++i;
  }
}

QStringList MultiFetcher::FetcherListWidget::uuids() const {
  QStringList uuids;
  foreach(QWidget* widget, mWidgetList) {
    FetcherItemWidget* item = static_cast<FetcherItemWidget*>(widget);
    QString uuid = item->fetcherUuid();
    if(!uuid.isEmpty()) {
      uuids << uuid;
    }
  }
  return uuids;
}

QWidget* MultiFetcher::FetcherListWidget::createWidget(QWidget* parent_) {
  FetcherItemWidget* w = new FetcherItemWidget(parent_);
  w->setFetchers(m_fetchers);
  connect(w, SIGNAL(signalModified()), SIGNAL(signalModified()));
  return w;
}

