/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include "videogamegeekfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../utils/string_utils.h"
#include "../core/tellico_strings.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTextCodec>
#include <QUrlQuery>

namespace {
  // a lot of overlap with boardgamegeekimporter.h
  static const int BGG_MAX_RETURNS_TOTAL = 10;
  // interchangeable with boardgamegeek.com
  static const char* BGG_SEARCH_URL  = "http://videogamegeek.com/xmlapi2/search";
  static const char* BGG_THING_URL  = "http://videogamegeek.com/xmlapi2/thing";
}

using namespace Tellico;
using Tellico::Fetch::VideoGameGeekFetcher;

VideoGameGeekFetcher::VideoGameGeekFetcher(QObject* parent_)
    : XMLFetcher(parent_) {
  setLimit(BGG_MAX_RETURNS_TOTAL);
  setXSLTFilename(QStringLiteral("boardgamegeek2tellico.xsl"));
}

VideoGameGeekFetcher::~VideoGameGeekFetcher() {
}

QString VideoGameGeekFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

// https://boardgamegeek.com/wiki/page/XML_API_Terms_of_Use
QString VideoGameGeekFetcher::attribution() const {
  return i18n(providedBy).arg(QLatin1String("https://boardgamegeek.com"), QLatin1String("BoardGameGeek"));
}

bool VideoGameGeekFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Keyword;
}

bool VideoGameGeekFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

QUrl VideoGameGeekFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(BGG_SEARCH_URL));

  QUrlQuery q;
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("query"), request().value());
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("videogame,videogameexpansion"));
      q.addQueryItem(QStringLiteral("exact"), QStringLiteral("1"));
      break;

    case Keyword:
      q.addQueryItem(QStringLiteral("query"), request().value());
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("videogame,videogameexpansion"));
      break;

    case Raw:
      u.setUrl(QLatin1String(BGG_THING_URL));
      q.addQueryItem(QStringLiteral("id"), request().value());
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("videogame,videogameexpansion"));
      break;

    default:
      myWarning() << "key not recognized: " << request().key();
      return QUrl();
  }
  u.setQuery(q);

//  myDebug() << "url: " << u.url();
  return u;
}

Tellico::Data::EntryPtr VideoGameGeekFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  const QString id = entry_->field(QStringLiteral("bggid"));
  if(id.isEmpty()) {
    myDebug() << "no bgg id found";
    return entry_;
  }

  QUrl u(QString::fromLatin1(BGG_THING_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("id"), id);
  q.addQueryItem(QStringLiteral("type"), QStringLiteral("videogame,videogameexpansion"));
  u.setQuery(q);
//  myDebug() << "url: " << u;

  // quiet
  QString output = FileHandler::readXMLFile(u, true);

#if 0
  myWarning() << "Remove output debug from videogamegeekfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-videogamegeek.xml"));
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
  if(coll->entryCount() == 0) {
    myWarning() << "no entries";
    return entry_;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QStringLiteral("bggid"));
  return coll->entries().front();
}

Tellico::Fetch::FetchRequest VideoGameGeekFetcher::updateRequest(Data::EntryPtr entry_) {
  QString bggid = entry_->field(QStringLiteral("bggid"));
  if(!bggid.isEmpty()) {
    return FetchRequest(Raw, bggid);
  }

  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* VideoGameGeekFetcher::configWidget(QWidget* parent_) const {
  return new VideoGameGeekFetcher::ConfigWidget(parent_, this);
}

QString VideoGameGeekFetcher::defaultName() {
  return QStringLiteral("VideoGameGeek");
}

QString VideoGameGeekFetcher::defaultIcon() {
  return favIcon("https://cf.geekdo-static.com/icons/favicon2.ico");
}

Tellico::StringHash VideoGameGeekFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("videogamegeek-link")] = i18n("VideoGameGeek Link");
  return hash;
}

VideoGameGeekFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const VideoGameGeekFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(VideoGameGeekFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void VideoGameGeekFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString VideoGameGeekFetcher::ConfigWidget::preferredName() const {
  return VideoGameGeekFetcher::defaultName();
}
