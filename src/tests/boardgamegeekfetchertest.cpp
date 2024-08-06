/***************************************************************************
    Copyright (C) 2010-2011 Robby Stephenson <robby@periapsis.org>
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

#include "boardgamegeekfetchertest.h"

#include "../fetch/boardgamegeekfetcher.h"
#include "../collections/boardgamecollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( BoardGameGeekFetcherTest )

BoardGameGeekFetcherTest::BoardGameGeekFetcherTest() : AbstractFetcherTest() {
}

void BoardGameGeekFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BoardGameCollection> registerBoard(Tellico::Data::Collection::BoardGame, "boardgame");
  Tellico::ImageFactory::init();
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/boardgamegeek2tellico.xsl"));
}

void BoardGameGeekFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::BoardGame, Tellico::Fetch::Title,
                                       QStringLiteral("Catan"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BoardGameGeekFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::BoardGame);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("CATAN"));
  QCOMPARE(entry->field(QStringLiteral("designer")), QStringLiteral("Klaus Teuber"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("publisher"))).at(0), QStringLiteral("KOSMOS"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1995"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("genre"))).at(0), QStringLiteral("Economic"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("mechanism"))).at(0), QStringLiteral("Chaining"));
  QCOMPARE(entry->field(QStringLiteral("num-player")), QStringLiteral("3; 4"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("description")).contains(QLatin1String("#10;")));
  QCOMPARE(entry->field(QStringLiteral("boardgamegeek-link")), QStringLiteral("https://www.boardgamegeek.com/boardgame/13"));
}

void BoardGameGeekFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::BoardGame, Tellico::Fetch::Keyword,
                                       QStringLiteral("The Settlers of Catan"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BoardGameGeekFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 10);
}

void BoardGameGeekFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BoardGameCollection(true));
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("bggid"), QStringLiteral("bggid")));
  coll->addField(field);
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);
  entry->setField(QStringLiteral("bggid"), QStringLiteral("13"));

  Tellico::Fetch::BoardGameGeekFetcher fetcher(this);
  auto request = fetcher.updateRequest(entry);
  request.setCollectionType(coll->type());
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QCOMPARE(request.value(), QStringLiteral("13"));

  Tellico::Data::EntryList results = DO_FETCH1(&fetcher, request, 1);
  QCOMPARE(results.size(), 1);

  entry = results.at(0);
  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::BoardGame);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("CATAN"));
  QCOMPARE(entry->field(QStringLiteral("designer")), QStringLiteral("Klaus Teuber"));
}
