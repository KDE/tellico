/***************************************************************************
    Copyright (C) 2013 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>
#include "vndbfetcher.h"
#include "../collections/gamecollection.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QTcpSocket>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace {
  static const char* VNDB_HOSTNAME = "api.vndb.org";
  static const int VNDB_PORT = 19534;
}

using namespace Tellico;
using Tellico::Fetch::VNDBFetcher;

VNDBFetcher::VNDBFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false), m_socket(nullptr), m_isConnected(false), m_state(PreLogin) {
}

VNDBFetcher::~VNDBFetcher() {
  delete m_socket;
  m_socket = nullptr;
}

QString VNDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool VNDBFetcher::canSearch(FetchKey k) const {
  return k == Title;
}

bool VNDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

void VNDBFetcher::readConfigHook(const KConfigGroup&) {
}

void VNDBFetcher::search() {
  m_started = true;
  m_data.clear();

  if(!m_socket) {
    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(readyRead()), SLOT(slotRead()));
    connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(slotState()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(slotError()));
  }
  if(!m_isConnected) {
    m_socket->connectToHost(QLatin1String(VNDB_HOSTNAME), VNDB_PORT);
  }

  //the client ver only wants digits, I think?
  QString clientVersion(QStringLiteral(TELLICO_VERSION));
  clientVersion.remove(QRegExp(QLatin1String("[^0-9.]")));

  QByteArray login = "login {"
                             "\"protocol\":1,"
                             "\"client\":\"Tellico\","
                             "\"clientver\": \"";
  login += clientVersion.toUtf8() + "\"}";
  login.append(0x04);
  if(m_socket->waitForConnected()) {
//    myDebug() << login;
    m_socket->write(login);
    m_socket->waitForReadyRead(5000);
    if(m_state == PreLogin) {
      // login did not work
      stop();
      return;
    }
  }

  QByteArray get = "get vn basic,details ";
  switch(request().key) {
    case Title:
      get += "(title ~ \"" + request().value.toUtf8() + "\")";
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      return;
  }

//  myDebug() << "get:" << get;
  get.append(0x04);
  m_socket->write(get);
}

void VNDBFetcher::stop() {
  if(!m_started) {
    return;
  }
  m_started = false;
  if(m_socket && m_socket->isValid()) {
//    myDebug() << "disconnecting";
    m_socket->disconnectFromHost();
    m_state = PreLogin;
  }
  emit signalDone(this);
}

Tellico::Data::EntryPtr VNDBFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // image might still be a URL
  const QString image_id = entry->field(QStringLiteral("cover"));
  if(image_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  return entry;
}

Tellico::Fetch::FetchRequest VNDBFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void VNDBFetcher::slotComplete() {
  if(m_data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  QByteArray data = m_data;
  // remove the last hex character
  data.chop(1);

  if(data.startsWith("error")) { //krazy:exclude=strings
    QJsonDocument doc = QJsonDocument::fromJson(data.mid(5));
    QVariantMap result = doc.object().toVariantMap();
    if(result.contains(QStringLiteral("msg"))) {
      myDebug() << result.value(QStringLiteral("msg")).toString();
      message(result.value(QStringLiteral("msg")).toString(), MessageHandler::Error);
    }
    stop();
    return;
  }

//  myDebug() << data;
  if(m_state == PreLogin) {
    if(data.startsWith("ok")) { //krazy:exclude=strings
      m_state = PostLogin;
      m_data.clear(); // reset data buffer
    } else {
      stop();
    }
    return;
  }

  if(!data.startsWith("results")) { //krazy:exclude=strings
    myDebug() << "Expecting results!";
    stop();
    return;
  }

  data = data.mid(7);

#if 0
  myWarning() << "Remove debug from vndbfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/vndbtest.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonParseError jsonError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
  if(doc.isNull()) {
    myDebug() << "null JSON document:" << jsonError.errorString();
    message(jsonError.errorString(), MessageHandler::Error);
    stop();
    return;
  }
  QVariantMap topResultMap = doc.object().toVariantMap();
  QVariantList resultList = topResultMap.value(QStringLiteral("items")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  Data::CollPtr coll(new Data::GameCollection(true));
  // add new fields
  if(optionalFields().contains(QLatin1String("origtitle"))) {
    Data::FieldPtr f(new Data::Field(QStringLiteral("origtitle"), i18n("Original Title")));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);
  }
  if(optionalFields().contains(QLatin1String("alias"))) {
    Data::FieldPtr f(new Data::Field(QStringLiteral("alias"), i18n("Alias")));
    f->setFlags(Data::Field::AllowMultiple);
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);
  }

  QVariantMap resultMap;
  foreach(const QVariant& result, resultList) {
    // be sure to check that the fetcher has not been stopped
    // crashes can occur if not
    if(!m_started) {
      break;
    }
    resultMap = result.toMap();

    Data::EntryPtr entry(new Data::Entry(coll));
    entry->setField(QStringLiteral("title"), mapValue(resultMap, "title"));
    entry->setField(QStringLiteral("year"), mapValue(resultMap, "released").left(4));
    entry->setField(QStringLiteral("genre"), i18n("Visual Novel"));
    entry->setField(QStringLiteral("description"), mapValue(resultMap, "description"));
    entry->setField(QStringLiteral("cover"), mapValue(resultMap, "image"));
    if(optionalFields().contains(QLatin1String("origtitle"))) {
      entry->setField(QStringLiteral("origtitle"), mapValue(resultMap, "original"));
    }
    if(optionalFields().contains(QLatin1String("alias"))) {
      const QString aliases = mapValue(resultMap, "aliases");
      entry->setField(QStringLiteral("alias"), aliases.split(QStringLiteral("\n")).join(FieldFormat::delimiterString()));
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

//  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches
  stop();
}

void VNDBFetcher::slotState() {
  if(!m_socket) {
    return;
  }
  if(m_socket->state() == QAbstractSocket::ConnectedState) {
    m_isConnected = true;
  } else if(m_socket->state() == QAbstractSocket::UnconnectedState) {
    m_isConnected = false;
  }
//  myDebug() << "state" << m_socket->state();
}

void VNDBFetcher::slotError() {
  if(!m_socket) {
    return;
  }
  myDebug() << m_socket->errorString();
}

void VNDBFetcher::slotRead() {
  m_data += m_socket->readAll();

  // check the last character, if it's not the hex character, continue waiting
  if(m_socket->atEnd() && m_data.endsWith(0x04)) {
    slotComplete();
  }
}


Tellico::Fetch::ConfigWidget* VNDBFetcher::configWidget(QWidget* parent_) const {
  return new VNDBFetcher::ConfigWidget(parent_, this);
}

QString VNDBFetcher::defaultName() {
  return QStringLiteral("Visual Novel Database"); // no translation
}

QString VNDBFetcher::defaultIcon() {
  return favIcon("http://www.vndb.org");
}

Tellico::StringHash VNDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("origtitle")] = i18n("Original Title");
  hash[QStringLiteral("alias")] = i18n("Alias");
  return hash;
}

VNDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const VNDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(VNDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void VNDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString VNDBFetcher::ConfigWidget::preferredName() const {
  return VNDBFetcher::defaultName();
}
