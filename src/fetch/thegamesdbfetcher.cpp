/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "thegamesdbfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
  static const char* THEGAMESDB_SEARCH_API_URL = "http://thegamesdb.net/api/GetGamesList.php";
  static const char* THEGAMESDB_DETAIL_API_URL = "http://thegamesdb.net/api/GetGame.php";
}

using namespace Tellico;
using Tellico::Fetch::TheGamesDBFetcher;

TheGamesDBFetcher::TheGamesDBFetcher(QObject* parent_)
    : XMLFetcher(parent_) {
  setLimit(10);
  setXSLTFilename(QLatin1String("thegamesdb2tellico.xsl"));
}

TheGamesDBFetcher::~TheGamesDBFetcher() {
}

QString TheGamesDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool TheGamesDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

KUrl TheGamesDBFetcher::searchUrl() {
  KUrl u;

  switch(request().key) {
    case Title:
      u = KUrl(THEGAMESDB_DETAIL_API_URL);
      u.addQueryItem(QLatin1String("name"), request().value);
      break;

    case Keyword:
      u = KUrl(THEGAMESDB_SEARCH_API_URL);
      u.addQueryItem(QLatin1String("name"), request().value);
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      return KUrl();
  }

//  myDebug() << "url: " << u.url();
  return u;
}

Tellico::Data::EntryPtr TheGamesDBFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  const QString id = entry_->field(QLatin1String("thegamesdb-id"));
  if(id.isEmpty()) {
    myDebug() << "no id found";
    return entry_;
  }

  KUrl u(THEGAMESDB_DETAIL_API_URL);
  u.addQueryItem(QLatin1String("id"), id);
//  myDebug() << "url: " << u;

  // quiet
  QString output = FileHandler::readXMLFile(u, true);

#if 0
  myWarning() << "Remove output debug from thegamesdbfetcher.cpp";
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << output;
  }
  f.close();
#endif

  Import::TellicoImporter imp(xsltHandler()->applyStylesheet(output));
  // be quiet when loading images
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  Data::CollPtr coll = imp.collection();
//  getTracks(entry);
  if(!coll) {
    myWarning() << "no collection pointer";
    return entry_;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QLatin1String("thegamesdb-id"));
  return coll->entries().front();
}

Tellico::Fetch::FetchRequest TheGamesDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* TheGamesDBFetcher::configWidget(QWidget* parent_) const {
  return new TheGamesDBFetcher::ConfigWidget(parent_, this);
}

QString TheGamesDBFetcher::defaultName() {
  return QLatin1String("TheGamesDB.net");
}

QString TheGamesDBFetcher::defaultIcon() {
  return favIcon("http://thegamesdb.net");
}

Tellico::StringHash TheGamesDBFetcher::allOptionalFields() {
  StringHash hash;
  return hash;
}

TheGamesDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const TheGamesDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(TheGamesDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void TheGamesDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString TheGamesDBFetcher::ConfigWidget::preferredName() const {
  return TheGamesDBFetcher::defaultName();
}

#include "thegamesdbfetcher.moc"
