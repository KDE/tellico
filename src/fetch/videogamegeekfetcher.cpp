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
  static const char* BGG_SEARCH_URL  = "http://boardgamegeek.com/xmlapi2/search";
  static const char* BGG_THING_URL  = "http://boardgamegeek.com/xmlapi2/thing";
}

using namespace Tellico;
using Tellico::Fetch::VideoGameGeekFetcher;

VideoGameGeekFetcher::VideoGameGeekFetcher(QObject* parent_)
    : XMLFetcher(parent_) {
  setLimit(BGG_MAX_RETURNS_TOTAL);
  setXSLTFilename(QLatin1String("boardgamegeek2tellico.xsl"));
}

VideoGameGeekFetcher::~VideoGameGeekFetcher() {
}

QString VideoGameGeekFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool VideoGameGeekFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

QUrl VideoGameGeekFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(BGG_SEARCH_URL));

  QUrlQuery q;
  switch(request().key) {
    case Title:
      q.addQueryItem(QLatin1String("query"), request().value);
      q.addQueryItem(QLatin1String("type"), QLatin1String("videogame,videogameexpansion"));
      q.addQueryItem(QLatin1String("exact"), QLatin1String("1"));
      break;

    case Keyword:
      q.addQueryItem(QLatin1String("query"), request().value);
      q.addQueryItem(QLatin1String("type"), QLatin1String("videogame,videogameexpansion"));
      break;

    case Raw:
      u.setUrl(QLatin1String(BGG_THING_URL));
      q.addQueryItem(QLatin1String("id"), request().value);
      q.addQueryItem(QLatin1String("type"), QLatin1String("videogame,videogameexpansion"));
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      return QUrl();
  }
  u.setQuery(q);

//  myDebug() << "url: " << u.url();
  return u;
}

Tellico::Data::EntryPtr VideoGameGeekFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  const QString id = entry_->field(QLatin1String("bggid"));
  if(id.isEmpty()) {
    myDebug() << "no bgg id found";
    return entry_;
  }

  QUrl u(QString::fromLatin1(BGG_THING_URL));
  QUrlQuery q;
  q.addQueryItem(QLatin1String("id"), id);
  q.addQueryItem(QLatin1String("type"), QLatin1String("videogame,videogameexpansion"));
  u.setQuery(q);
//  myDebug() << "url: " << u;

  // quiet
  QString output = FileHandler::readXMLFile(u, true);

#if 0
  myWarning() << "Remove output debug from videogamegeekfetcher.cpp";
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
  if(coll->entryCount() == 0) {
    myWarning() << "no entries";
    return entry_;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QLatin1String("bggid"));
  return coll->entries().front();
}

Tellico::Fetch::FetchRequest VideoGameGeekFetcher::updateRequest(Data::EntryPtr entry_) {
  QString bggid = entry_->field(QLatin1String("bggid"));
  if(!bggid.isEmpty()) {
    return FetchRequest(Raw, bggid);
  }

  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* VideoGameGeekFetcher::configWidget(QWidget* parent_) const {
  return new VideoGameGeekFetcher::ConfigWidget(parent_, this);
}

QString VideoGameGeekFetcher::defaultName() {
  return QLatin1String("VideoGameGeek");
}

QString VideoGameGeekFetcher::defaultIcon() {
  return favIcon("http://www.videogamegeek.com");
}

Tellico::StringHash VideoGameGeekFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("videogamegeek-link")] = i18n("VideoGameGeek Link");
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
