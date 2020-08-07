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

IGDBFetcherTest::IGDBFetcherTest() : AbstractFetcherTest()
    , m_config(QFINDTESTDATA("tellicotest_private.config"), KConfig::SimpleConfig) {
}

void IGDBFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest_private.config"));
}

void IGDBFetcherTest::testKeyword() {
  const QString groupName = QStringLiteral("IGDB");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with IGDB settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Keyword,
                                       QStringLiteral("Zelda Twilight Princess"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IGDBFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 5);

  QVERIFY(!results.isEmpty());
  // want the Wii version
  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr e, results) {
    if(e->field("platform") == QStringLiteral("Nintendo Wii")) {
      entry = e;
      break;
    }
  }

  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Legend of Zelda: Twilight Princess"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("platform"), QStringLiteral("Nintendo Wii"));
  QCOMPARE(entry->field("certification"), QStringLiteral("Teen"));
  QCOMPARE(entry->field("pegi"), QStringLiteral("PEGI 12"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Role-playing (RPG); Adventure"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Nintendo"));
  QCOMPARE(entry->field("developer"), QStringLiteral("Nintendo EAD Group No. 3"));
  QCOMPARE(entry->field("igdb"), QStringLiteral("https://www.igdb.com/games/the-legend-of-zelda-twilight-princess--1"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
