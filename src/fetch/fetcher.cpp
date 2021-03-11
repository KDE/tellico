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

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KIO/Global>
#include <KIO/SlaveConfig>
#include <kio_version.h>
#if KIO_VERSION >= QT_VERSION_CHECK(5,19,0)
#include <KIO/FavIconRequestJob>
#endif

#include <QUrl>
#include <QUuid>
#include <QPointer>

using namespace Tellico::Fetch;
using Tellico::Fetch::Fetcher;

Fetcher::Fetcher(QObject* parent) : QObject(parent)
    , m_updateOverwrite(false)
    , m_hasMoreResults(false)
    , m_messager(nullptr) {
}

Fetcher::~Fetcher() {
  saveConfig();
}

int Fetcher::collectionType() const {
  return m_request.collectionType();
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
  if(!canFetch(m_request.collectionType())) {
    myDebug() << "Bad collection type:" << source() << m_request.collectionType();
    message(i18n("%1 does not allow searching for this collection type.", source()),
            MessageHandler::Warning);
    emit signalDone(this);
    return;
  }

  if(needsUserAgent()) {
    // copied from KProtocolManager::userAgentForHost
    const QString sendUserAgent = KIO::SlaveConfig::self()->configData(QStringLiteral("http"),
                                                                       QString(), // host name
                                                                       QStringLiteral("SendUserAgent")).toLower();
    if(sendUserAgent == QLatin1String("false")) {
      myDebug() << "Fetcher - Need user agent for" << source();
      // TODO: I'd like to link to kcmshell5 useragent as the Konqueror about page does
      // but KIO complains about unable to open exec links
//      message(i18n("<html>%1 requires the network request to include identification.\n"
//                   "Check the network configuration in <a href=\"%2\">KDE System Settings</a>.</html>", source(), QStringLiteral("exec:/kcmshell5 useragent")),
      message(i18n("%1 requires the network request to include identification.\n"
                   "Check the network configuration in KDE System Settings.", source()),
              MessageHandler::Error);
      emit signalDone(this);
      return;
    }
  }

  m_entries.clear();
  search();
}

void Fetcher::startUpdate(Tellico::Data::EntryPtr entry_) {
  Q_ASSERT(entry_);
  Q_ASSERT(entry_->collection());
  m_request = updateRequest(entry_);
  m_request.setCollectionType(entry_->collection()->type());
  if(m_request.isNull()) {
    myLog() << "insufficient info to update" << entry_->title();
    emit signalDone(this); // always need to emit this if not continuing with the search
    return;
  }
  search();
}

void Fetcher::readConfig(const KConfigGroup& config_) {
  m_configGroup = config_;

  QString s = config_.readEntry("Name");
  if(!s.isEmpty()) {
    m_name = s;
  }
  m_updateOverwrite = config_.readEntry("UpdateOverwrite", false);
  // it's called custom fields here, but it's really optional lists
  m_fields = config_.readEntry("Custom Fields", QStringList());
  s = config_.readEntry("Uuid");
  if(s.isEmpty()) {
    s = QUuid::createUuid().toString();
  }
  m_uuid = s;
  // be sure to read config for subclass
  readConfigHook(config_);
}

void Fetcher::saveConfig() {
  if(!m_configGroup.isValid() || m_configGroup.isImmutable()) {
    return;
  }
  m_configGroup.writeEntry("Uuid", m_uuid);
  saveConfigHook(m_configGroup);
  m_configGroup.sync();
}

void Fetcher::setConfigGroup(const KConfigGroup& group_) {
  m_configGroup = group_;
}

Tellico::Data::EntryPtr Fetcher::fetchEntry(uint uid_) {
  // check if already fetched this entry
  if(m_entries.contains(uid_)) {
    return m_entries[uid_];
  }

  QPointer<Fetcher> ptr(this);
  Data::EntryPtr entry = fetchEntryHook(uid_);
  // could be cancelled and killed after fetching entry, check ptr
  if(ptr && entry) {
    // iterate over list of possible optional fields
    // and if the field is not included in the user-configured list
    // remove the field from the entry
    QHashIterator<QString, QString> i(Manager::optionalFields(type()));
    while(i.hasNext()) {
      i.next();
      if(!m_fields.contains(i.key())) {
        entry->collection()->removeField(i.key());
      }
    }
  }
  m_entries.insert(uid_, entry);
  return entry;
}

void Fetcher::setMessageHandler(MessageHandler* handler) {
  m_messager = handler;
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
  return favIcon(QUrl(QString::fromLatin1(url_)));
}

QString Fetcher::favIcon(const QUrl& url_) {
  if(!url_.isValid()) {
    return QString();
  }

#if KIO_VERSION >= QT_VERSION_CHECK(5,19,0)
  KIO::FavIconRequestJob* job = new KIO::FavIconRequestJob(url_);
  // if the url has a meaningful path, then use it as the icon url
  if(url_.path().size() > 4 && url_.path().contains(QLatin1Char('.'))) {
    job->setIconUrl(url_);
  }
/*
  connect(job, &KIO::FavIconRequestJob::result, [job](KJob *) {
         if(job->error()) {
           myDebug() << "error:" << job->errorString();
         }
     });
*/
#endif
  QString name = KIO::favIconForUrl(url_);

  // favIcons start with "/". being an absolute file path from FavIconFetchJob
  // but KIconLoader still expects them to start with "favicons/" and appends ".png"
  // since the rest of Tellico assumes KDE4 behavior, adjust here
  if(name.startsWith(QLatin1Char('/'))) {
    int pos = name.indexOf(QLatin1String("favicons/"));
    if(pos > -1) {
      name = name.mid(pos);
      name.chop(4); // remove ".png";
    }
  }
  return name;
}
