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
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <klocale.h>

#include <QTcpSocket>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>

#ifdef HAVE_QJSON
#include <qjson/parser.h>
#include <qjson/serializer.h>
#endif

namespace {
  static const char* VNDB_HOSTNAME = "api.vndb.org";
  static const int VNDB_PORT = 19534;
}

using namespace Tellico;
using Tellico::Fetch::VNDBFetcher;

VNDBFetcher::VNDBFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false), m_socket(0), m_isConnected(false), m_state(PreLogin) {
}

VNDBFetcher::~VNDBFetcher() {
  delete m_socket;
  m_socket = 0;
}

QString VNDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

// without QJSON, we can only search on ISBN for covers
bool VNDBFetcher::canSearch(FetchKey k) const {
#ifdef HAVE_QJSON
  return k == Title;
#else
  return false;
#endif
}

bool VNDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

void VNDBFetcher::readConfigHook(const KConfigGroup&) {
}

void VNDBFetcher::search() {
  m_started = true;
#ifndef HAVE_QJSON
  stop();
  return;
#else

  if(!m_socket) {
    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(readyRead()), SLOT(slotComplete()));
    connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(slotState()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(slotError()));
  }
  if(!m_isConnected) {
    m_socket->connectToHost(QLatin1String(VNDB_HOSTNAME), VNDB_PORT);
  }

  //the client ver only wants digits, I think?
  QString clientVersion(QLatin1String(TELLICO_VERSION));
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
  QJson::Serializer serializer;
  switch(request().key) {
    case Title:
      get += "(title ~ " + serializer.serialize(request().value) + ')';
//      u.addQueryItem(QLatin1String("title"), term_);
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      return;
  }

//  myDebug() << "get:" << get;
  get.append(0x04);
  m_socket->write(get);
#endif
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

  return entry;
}

Tellico::Fetch::FetchRequest VNDBFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void VNDBFetcher::slotComplete() {
#ifdef HAVE_QJSON
//  myDebug();

  QByteArray data = m_socket->readAll();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  // remove the late hex character
  data.chop(1);

  if(data.startsWith("error")) {
    QJson::Parser parser;
    QVariantMap result = parser.parse(data.mid(5)).toMap();
    if(result.contains(QLatin1String("msg"))) {
      myDebug() << result.value(QLatin1String("msg")).toString();
      message(result.value(QLatin1String("msg")).toString(), MessageHandler::Error);
    }
    stop();
    return;
  }

//  myDebug() << data;
  if(m_state == PreLogin) {
    if(data.startsWith("ok")) {
      m_state = PostLogin;
    } else {
      stop();
    }
    return;
  }

  if(!data.startsWith("results")) {
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

  QJson::Parser parser;
  QVariantMap topResultMap = parser.parse(data).toMap();
  QVariantList resultList = topResultMap.value(QLatin1String("items")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  Data::CollPtr coll(new Data::GameCollection(true));
  // add new fields
  if(optionalFields().contains(QLatin1String("origtitle"))) {
    Data::FieldPtr f(new Data::Field(QLatin1String("origtitle"), i18n("Original Title")));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);
  }
  if(optionalFields().contains(QLatin1String("alias"))) {
    Data::FieldPtr f(new Data::Field(QLatin1String("alias"), i18n("Alias")));
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
    entry->setField(QLatin1String("title"), value(resultMap, "title"));
    entry->setField(QLatin1String("year"), value(resultMap, "released").left(4));
    entry->setField(QLatin1String("genre"), i18n("Visual Novel"));
    entry->setField(QLatin1String("description"), value(resultMap, "description"));
    entry->setField(QLatin1String("cover"), value(resultMap, "image"));
    if(optionalFields().contains(QLatin1String("origtitle"))) {
      entry->setField(QLatin1String("origtitle"), value(resultMap, "original"));
    }
    if(optionalFields().contains(QLatin1String("alias"))) {
      const QString aliases = value(resultMap, "aliases");
      entry->setField(QLatin1String("alias"), aliases.split(QLatin1String("\n")).join(FieldFormat::delimiterString()));
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

//  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches
#endif
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

Tellico::Fetch::ConfigWidget* VNDBFetcher::configWidget(QWidget* parent_) const {
  return new VNDBFetcher::ConfigWidget(parent_, this);
}

QString VNDBFetcher::defaultName() {
  return QLatin1String("Visual Novel Database"); // no translation
}

QString VNDBFetcher::defaultIcon() {
  return favIcon("http://www.vndb.org");
}

Tellico::StringHash VNDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("origtitle")] = i18n("Original Title");
  hash[QLatin1String("alias")] = i18n("Alias");
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

// static
QString VNDBFetcher::value(const QVariantMap& map, const char* name) {
  const QVariant v = map.value(QLatin1String(name));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::String)) {
    return v.toString();
  } else if(v.canConvert(QVariant::StringList)) {
    return v.toStringList().join(Tellico::FieldFormat::delimiterString());
  } else if(v.canConvert(QVariant::Map)) {
    return v.toMap().value(QLatin1String("value")).toString();
  } else {
    return QString();
  }
}

