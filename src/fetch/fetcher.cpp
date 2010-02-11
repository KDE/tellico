/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "fetcher.h"
#include "fetchmanager.h" // for calling static optional fields
#include "../collection.h"
#include "../entry.h"
#include "../tellico_debug.h"

#include <kglobal.h>
#include <klocale.h>
#include <KSharedConfig>
#include <KConfigGroup>
#include <kmimetype.h>

#include <QDBusInterface>
#include <QDBusReply>

using namespace Tellico::Fetch;
using Tellico::Fetch::Fetcher;

Fetcher::Fetcher(QObject* parent) : QObject(parent)
    , QSharedData()
    , m_updateOverwrite(false)
    , m_hasMoreResults(false)
    , m_messager(0) {
}

Fetcher::~Fetcher() {
  KConfigGroup config(KGlobal::config(), m_configGroup);
  saveConfigHook(config);
}

int Fetcher::collectionType() const {
  return m_request.collectionType;
}

/// virtual, overridden by subclasses
bool Fetcher::canUpdate() const {
  return true;
}

bool Fetcher::updateOverwrite() const {
  return m_updateOverwrite;
}

const Tellico::Fetch::FetchRequest& Fetcher::request() const {
  return m_request;
}

void Fetcher::startSearch(const FetchRequest& request_) {
  m_request = request_;
  if(!canFetch(m_request.collectionType)) {
    message(i18n("%1 does not allow searching for this collection type.", source()), MessageHandler::Warning);
    emit signalDone(this);
    return;
  }

  search();
}

void Fetcher::startUpdate(Tellico::Data::EntryPtr entry_) {
  Q_ASSERT(entry_);
  Q_ASSERT(entry_->collection());
  m_request = updateRequest(entry_);
  m_request.collectionType = entry_->collection()->type();
  if(!m_request.isNull()) {
    search();
  } else {
    myDebug() << "insufficient info to search";
    emit signalDone(this); // always need to emit this if not continuing with the search
  }
//  updateEntry(entry_);
}

void Fetcher::readConfig(const KConfigGroup& config_, const QString& groupName_) {
  m_configGroup = groupName_;

  QString s = config_.readEntry("Name");
  if(!s.isEmpty()) {
    m_name = s;
  }
  m_updateOverwrite = config_.readEntry("UpdateOverwrite", false);
  // it's called custom fields here, but it's really optional lists
  m_fields = config_.readEntry("Custom Fields", QStringList());
  // be sure to read config for subclass
  readConfigHook(config_);
}

Tellico::Data::EntryPtr Fetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = fetchEntryHook(uid_);
  if(entry) {
    // iterate over list of possible optional fields
    // and if the field is not included in the user-configured list
    // remove the field from the entry
    foreach(const QString& field, Manager::optionalFields(type()).keys()) {
      if(!m_fields.contains(field)) {
        entry->collection()->removeField(field);
      }
    }
  }
  return entry;
}

void Fetcher::message(const QString& message_, int type_) const {
  if(m_messager) {
    m_messager->send(message_, static_cast<MessageHandler::Type>(type_));
  }
}

void Fetcher::infoList(const QString& message_, const QStringList& list_) const {
  if(m_messager) {
    m_messager->infoList(message_, list_);
  }
}

QString Fetcher::favIcon(const char* url_) {
  return favIcon(KUrl(url_));
}

QString Fetcher::favIcon(const KUrl& url_) {
  QDBusInterface kded(QLatin1String("org.kde.kded"),
                      QLatin1String("/modules/favicons"),
                      QLatin1String("org.kde.FavIcon"));
  if(!kded.isValid()) {
    myDebug() << "invalid dbus interface";
  }
  QDBusReply<QString> iconName = kded.call(QLatin1String("iconForUrl"), url_.url());
  if(iconName.isValid() && !iconName.value().isEmpty()) {
    return iconName;
  }
  // go ahead and try to download it for later
  kded.call(QLatin1String("downloadHostIcon"), url_.url());
  return KMimeType::iconNameForUrl(url_);
}

#include "fetcher.moc"
