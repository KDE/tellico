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

#undef QT_NO_CAST_FROM_ASCII

#include "videogamegeekfetchertest.h"

#include "../fetch/videogamegeekfetcher.h"
#include "../collections/gamecollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( VideoGameGeekFetcherTest )

VideoGameGeekFetcherTest::VideoGameGeekFetcherTest() : AbstractFetcherTest() {
}

void VideoGameGeekFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::GameCollection> registerVGG(Tellico::Data::Collection::Game, "game");
  Tellico::ImageFactory::init();
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/boardgamegeek2tellico.xsl"));
}

void VideoGameGeekFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Keyword,
                                       QStringLiteral("Mass Effect 3 Citadel"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::VideoGameGeekFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Game);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Mass Effect 3 - Citadel"));
  QCOMPARE(entry->field(QStringLiteral("developer")), QStringLiteral("BioWare"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Electronic Arts Inc. (EA)"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2013"));
//  QCOMPARE(entry->field(QStringLiteral("platform")), QStringLiteral("PlayStation3"));
  QCOMPARE(set(entry, "genre"), set("Action RPG; Shooter"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("description")).contains(QLatin1String("#10;")));
  QCOMPARE(entry->field(QStringLiteral("videogamegeek-link")), QStringLiteral("https://www.videogamegeek.com/videogame/139806"));
}

void VideoGameGeekFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::GameCollection(true));
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("bggid"), QStringLiteral("bggid")));
  coll->addField(field);
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);
  entry->setField(QStringLiteral("bggid"), QStringLiteral("139806"));

  Tellico::Fetch::VideoGameGeekFetcher fetcher(this);
  auto request = fetcher.updateRequest(entry);
  request.setCollectionType(coll->type());
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QCOMPARE(request.value(), QStringLiteral("139806"));

  Tellico::Data::EntryList results = DO_FETCH1(&fetcher, request, 1);
  QCOMPARE(results.size(), 1);

  entry = results.at(0);
  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Game);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Mass Effect 3 - Citadel"));
  QCOMPARE(entry->field(QStringLiteral("developer")), QStringLiteral("BioWare"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Electronic Arts Inc. (EA)"));
}
