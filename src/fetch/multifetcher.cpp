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
#include "../utils/mergeconflictresolver.h"
#include "../gui/collectiontypecombo.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

using namespace Tellico;
using Tellico::Fetch::MultiFetcher;

MultiFetcher::MultiFetcher(QObject* parent_)
    : Fetcher(parent_), m_collType(0), m_fetcherIndex(0), m_resultIndex(0), m_started(false) {
}

MultiFetcher::~MultiFetcher() {
}

QString MultiFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool MultiFetcher::canFetch(int type) const {
  return type == m_collType;
}

bool MultiFetcher::canSearch(Fetch::FetchKey k) const {
  // can fetch anything supported by the first data source
  // ensure we populate the child fetcher list before querying
  readSources();
  return !m_fetchers.isEmpty() && m_fetchers.front()->canSearch(k);
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
//      myDebug() << "Adding fetcher:" << fetcher->source();
      m_fetchers.append(fetcher);
      // in the event we have multiple instances of same fetcher, only need to connect once
      if(m_fetchers.count(fetcher) == 1) {
        connect(fetcher.data(), &Fetcher::signalResultFound,
                this, &MultiFetcher::slotResult);
        connect(fetcher.data(), &Fetcher::signalDone,
                this, &MultiFetcher::slotDone);
      }
    }
  }
}

void MultiFetcher::search() {
  m_started = true;
  readSources();
  if(m_fetchers.isEmpty()) {
//    myDebug() << source() << "has no sources";
    slotDone();
    return;
  }
  m_matches.clear();
//  myDebug() << "Starting" << m_fetchers.front()->source();
  m_fetchers.front()->startSearch(request());
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
  // if fetcher index is 0, this is the first set of results, save them all
  if(m_fetcherIndex == 0) {
//    myDebug() << "...found new result:" << newEntry->title();
    m_entries.append(newEntry);
    return;
  }

  // otherwise, keep the entry to compare later
  m_matches.append(newEntry);
}

void MultiFetcher::slotDone() {
  if(m_fetcherIndex == 0) {
    m_fetcherIndex++;
    m_resultIndex = 0;
  } else {
    // iterate over all the matches from this data source and figure out which one is the best match to the existing result
    Data::EntryPtr entry = m_entries.at(m_resultIndex);
    int bestScore = -1;
    int bestIndex = -1;
    for(int idx = 0; idx < m_matches.count(); ++idx) {
      auto match = m_matches.at(idx);
      const int score = entry->collection()->sameEntry(entry, match);
      if(score > bestScore) {
        bestScore = score;
        bestIndex = idx;
      }
      if(score >= EntryComparison::ENTRY_PERFECT_MATCH) {
        // no need to compare further
        break;
      }
    }
//    myDebug() << "best score" << bestScore  << "; index:" << bestIndex;
    if(bestIndex > -1 && bestScore >= EntryComparison::ENTRY_GOOD_MATCH) {
      auto newEntry = m_matches.at(bestIndex);
//      myDebug() << "...merging from" << newEntry->title() << "into" << entry->title();
      Merge::mergeEntry(entry, newEntry);
    } else {
//      myDebug() << "___no match for" << entry->title();
    }

    // now, bump to next result and continue trying to match
    m_resultIndex++;
    if(m_resultIndex >= m_entries.count()) {
      m_fetcherIndex++;
      m_resultIndex = 0;
    }
  }

  if(m_fetcherIndex < m_fetchers.count() && m_resultIndex < m_entries.count()) {
    Fetcher::Ptr fetcher = m_fetchers.at(m_fetcherIndex);
    Q_ASSERT(fetcher);
//    myDebug() << "updating entry#" << m_resultIndex << "from" << fetcher->source();
    fetcher->startUpdate(m_entries.at(m_resultIndex));
    return;
  }

  // at this point, all the fetchers have run through all the results, so we're
  // done so emit all results
  foreach(Data::EntryPtr entry, m_entries) {
    FetchResult* r = new FetchResult(this, entry);
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
  const QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  const QString title = entry_->field(QStringLiteral("title"));
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
  return QStringLiteral("folder-favorites");
}

Tellico::StringHash MultiFetcher::allOptionalFields() {
  StringHash hash;
  return hash;
}

MultiFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MultiFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());

  QWidget* hbox = new QWidget(optionsWidget());
  QHBoxLayout* hboxHBoxLayout = new QHBoxLayout(hbox);
  hboxHBoxLayout->setContentsMargins(0, 0, 0, 0);
  l->addWidget(hbox);

  QLabel* label = new QLabel(i18n("Collection &type:"), hbox);
  hboxHBoxLayout->addWidget(label);
  m_collCombo = new GUI::CollectionTypeCombo(hbox);
  void (GUI::CollectionTypeCombo::* activatedInt)(int) = &GUI::CollectionTypeCombo::activated;
  connect(m_collCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  connect(m_collCombo, activatedInt, this, &ConfigWidget::slotTypeChanged);
  label->setBuddy(m_collCombo);
  hboxHBoxLayout->addWidget(m_collCombo);

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
    : QFrame(parent_) {
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  setLayout(layout);

  QLabel* label = new QLabel(i18n("Data source:"), this);
  layout->addWidget(label);
  m_fetcherCombo = new GUI::ComboBox(this);
  layout->addWidget(m_fetcherCombo);
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_fetcherCombo, activatedInt, this, &FetcherItemWidget::signalModified);
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
    m_fetcherCombo->addItem(Manager::self()->fetcherIcon(fetcher.data()), fetcher->source(), fetcher->uuid());
  }
}

void MultiFetcher::FetcherItemWidget::setSource(Fetch::Fetcher::Ptr fetcher_) {
  m_fetcherCombo->setCurrentData(fetcher_->uuid());
}

QString MultiFetcher::FetcherItemWidget::fetcherUuid() const {
  return m_fetcherCombo->currentData().toString();
}

MultiFetcher::FetcherListWidget::FetcherListWidget(QWidget* parent_)
    : KWidgetLister(1, 8, parent_) {
  connect(this, &KWidgetLister::clearWidgets, this, &FetcherListWidget::signalModified);
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
  connect(w, &FetcherItemWidget::signalModified, this, &FetcherListWidget::signalModified);
  return w;
}
