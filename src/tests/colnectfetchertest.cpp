/***************************************************************************
    Copyright (C) 2019 Robby Stephenson <robby@periapsis.org>
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

#include "colnectfetchertest.h"

#include "../fetch/colnectfetcher.h"
#include "../entry.h"
#include "../collections/coincollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"
#include "../fetch/fetcherjob.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( ColnectFetcherTest )

ColnectFetcherTest::ColnectFetcherTest() : AbstractFetcherTest() {
}

void ColnectFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::CoinCollection> registerMe(Tellico::Data::Collection::Coin, "coin");
}

void ColnectFetcherTest::testSlug() {
  // test the implementation of the Colnect slug derivation
  QFETCH(QString, input);
  QFETCH(QString, slug);

  QCOMPARE(Tellico::Fetch::ColnectFetcher::URLize(input), slug);
}

void ColnectFetcherTest::testSlug_data() {
  QTest::addColumn<QString>("input");
  QTest::addColumn<QString>("slug");

  QTest::newRow("basic") << QStringLiteral("input") << QStringLiteral("input");
  QTest::newRow("Aus1$") << QStringLiteral("1 Dollar (50 Years Moonlanding)") << QStringLiteral("1_Dollar_50_Years_Moonlanding");
}

void ColnectFetcherTest::testRaw() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("colnect");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Coin,
                                       Tellico::Fetch::Raw,
                                       QStringLiteral("147558"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ColnectFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2019"));
  QCOMPARE(entry->field(QStringLiteral("country")), QStringLiteral("Australia"));
  QCOMPARE(entry->field(QStringLiteral("denomination")), QStringLiteral("$1.00"));
  QCOMPARE(entry->field(QStringLiteral("currency")), QStringLiteral("$ - Australian dollar"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("1970~Today - Numismatic Products"));
  QCOMPARE(entry->field(QStringLiteral("mintage")), QStringLiteral("25000"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("obverse")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("obverse")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("reverse")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("reverse")).contains(QLatin1Char('/')));
}

void ColnectFetcherTest::testSacagawea() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("colnect");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Coin,
                                       Tellico::Fetch::Keyword,
                                       QStringLiteral("2007 Sacagawea"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ColnectFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2007"));
  QCOMPARE(entry->field(QStringLiteral("country")), QStringLiteral("United States of America"));
  QCOMPARE(entry->field(QStringLiteral("denomination")), QStringLiteral("$1.00"));
  QCOMPARE(entry->field(QStringLiteral("currency")), QStringLiteral("$ - United States dollar"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("B06a - Eisenhower, Anthony & Sacagawea Dollar"));
  QCOMPARE(entry->field(QStringLiteral("mintage")), QStringLiteral("1497251077"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("obverse")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("obverse")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("reverse")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("reverse")).contains(QLatin1Char('/')));
}

void ColnectFetcherTest::testSkylab() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("colnect stamps");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Stamp,
                                       Tellico::Fetch::Title,
                                       QStringLiteral("2013 Skylab"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ColnectFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2013"));
  QCOMPARE(entry->field(QStringLiteral("country")), QStringLiteral("Papua New Guinea"));
  QCOMPARE(entry->field(QStringLiteral("stanley-gibbons")), QStringLiteral("PG 1638"));
  QCOMPARE(entry->field(QStringLiteral("michel")), QStringLiteral("PG 1902"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("15th Anniversary of Launch of International Space Station"));
  QCOMPARE(entry->field(QStringLiteral("gummed")), QStringLiteral("PVA (Polyvinyl Alcohol)"));
  QCOMPARE(entry->field(QStringLiteral("denomination")), QStringLiteral("K1.30"));
  QCOMPARE(entry->field(QStringLiteral("currency")), QStringLiteral("K - Papua New Guinean kina"));
  QCOMPARE(entry->field(QStringLiteral("color")), QStringLiteral("Multicolor"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("image")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("image")).contains(QLatin1Char('/')));
}
