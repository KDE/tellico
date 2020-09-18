/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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

#include "numistafetchertest.h"

#include "../fetch/numistafetcher.h"
#include "../collections/coincollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( NumistaFetcherTest )

NumistaFetcherTest::NumistaFetcherTest() : AbstractFetcherTest() {
}

void NumistaFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void NumistaFetcherTest::testSacagawea() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("numista");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Coin,
                                       Tellico::Fetch::Keyword,
                                       QStringLiteral("2019 Sacagawea"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::NumistaFetcher(this));
  fetcher->readConfig(cg, cg.name());

  static_cast<Tellico::Fetch::NumistaFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("type")), QStringLiteral("Native American Dollar"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2019"));
  QCOMPARE(entry->field(QStringLiteral("country")), QStringLiteral("United States"));
  QCOMPARE(entry->field(QStringLiteral("denomination")), QStringLiteral("1 Dollar"));
  QCOMPARE(entry->field(QStringLiteral("currency")), QStringLiteral("Dollar"));
  QCOMPARE(entry->field(QStringLiteral("numista")), QStringLiteral("https://en.numista.com/catalogue/pieces155679.html"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("obverse")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("obverse")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("reverse")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("reverse")).contains(QLatin1Char('/')));
}

void NumistaFetcherTest::testJefferson() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Coin,
                                       Tellico::Fetch::Keyword,
                                       QStringLiteral("1974 jefferson nickel"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::NumistaFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(!results.isEmpty());
  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("type")), QStringLiteral("Jefferson Nickel"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1974"));
  QCOMPARE(entry->field(QStringLiteral("country")), QStringLiteral("United States"));
  QCOMPARE(entry->field(QStringLiteral("denomination")), QStringLiteral("5 Cents"));
  QCOMPARE(entry->field(QStringLiteral("currency")), QStringLiteral("Dollar"));
}
