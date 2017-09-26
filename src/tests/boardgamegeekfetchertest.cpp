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

#include "../fetch/execexternalfetcher.h"
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
                                       QLatin1String("Catan"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BoardGameGeekFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::BoardGame);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Catan"));
  QCOMPARE(entry->field(QLatin1String("designer")), QLatin1String("Klaus Teuber"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field(QLatin1String("publisher"))).at(0), QLatin1String("KOSMOS"));
  QCOMPARE(entry->field(QLatin1String("year")), QLatin1String("1995"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field(QLatin1String("genre"))).at(0), QLatin1String("Negotiation"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field(QLatin1String("mechanism"))).at(0), QLatin1String("Dice Rolling"));
  QCOMPARE(entry->field(QLatin1String("num-player")), QLatin1String("3; 4"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QLatin1String("description")).isEmpty());
}

void BoardGameGeekFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::BoardGame, Tellico::Fetch::Keyword,
                                       QLatin1String("The Settlers of Catan"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BoardGameGeekFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 10);
}
