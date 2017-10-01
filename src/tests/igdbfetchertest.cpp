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

#include "igdbfetchertest.h"

#include "../fetch/igdbfetcher.h"
#include "../collections/gamecollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( IGDBFetcherTest )

IGDBFetcherTest::IGDBFetcherTest() : AbstractFetcherTest() {
}

void IGDBFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void IGDBFetcherTest::testKeyword() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QLatin1String("igdb");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Keyword,
                                       QLatin1String("Zelda Twilight Princess Wii"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IGDBFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QLatin1String("The Legend of Zelda: Twilight Princess"));
  QCOMPARE(entry->field("year"), QLatin1String("2006"));
  QCOMPARE(entry->field("platform"), QLatin1String("Nintendo Wii"));
  QCOMPARE(entry->field("certification"), QLatin1String("Teen"));
  QCOMPARE(entry->field("pegi"), QLatin1String("PEGI 12"));
  QCOMPARE(entry->field("genre"), QLatin1String("Platform; Puzzle; Role-playing (RPG); Adventure"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Nintendo"));
  QCOMPARE(entry->field("developer"), QLatin1String("Nintendo EAD Group No. 3"));
  QCOMPARE(entry->field("igdb"), QLatin1String("https://www.igdb.com/games/the-legend-of-zelda-twilight-princess"));
  QVERIFY(!entry->field(QLatin1String("description")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).contains(QLatin1Char('/')));
}
