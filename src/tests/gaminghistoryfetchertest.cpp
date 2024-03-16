/***************************************************************************
    Copyright (C) 2022 Robby Stephenson <robby@periapsis.org>
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

#include "gaminghistoryfetchertest.h"

#include "../fetch/gaminghistoryfetcher.h"
#include "../entry.h"
#include "../collections/gamecollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( GamingHistoryFetcherTest )

GamingHistoryFetcherTest::GamingHistoryFetcherTest() : AbstractFetcherTest() {
}

void GamingHistoryFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::GameCollection> registerVideo(Tellico::Data::Collection::Game, "game");
}

void GamingHistoryFetcherTest::testKeyword() {
  auto config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("vndb"));
  config.writeEntry("Custom Fields", QStringLiteral("gaming-history"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Keyword, QStringLiteral("Ikari Warriors 1986"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GamingHistoryFetcher(this));
  fetcher->readConfig(config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("Ikari Warriors"));
  QCOMPARE(entry->field("year"), QStringLiteral("1986"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Tradewest, Inc."));
  QCOMPARE(entry->field("platform"), QStringLiteral("Arcade Video"));
  QVERIFY(entry->field("description").startsWith(QLatin1String("Export release")));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field("gaming-history").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void GamingHistoryFetcherTest::testSingleResult() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Keyword, QStringLiteral("96 Flag Rally"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GamingHistoryFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("'96 Flag Rally"));
  QCOMPARE(entry->field("platform"), QStringLiteral("Arcade Video"));
}
