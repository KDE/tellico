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

#include "mobygamesfetchertest.h"

#include "../fetch/mobygamesfetcher.h"
#include "../collections/gamecollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( MobyGamesFetcherTest )

MobyGamesFetcherTest::MobyGamesFetcherTest() : AbstractFetcherTest()
    , m_config(QFINDTESTDATA("tellicotest_private.config"), KConfig::SimpleConfig) {
}

void MobyGamesFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest_private.config"));
}

void MobyGamesFetcherTest::testTitle() {
  const QString groupName = QStringLiteral("MobyGames");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with MobyGames settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Title,
                                       QStringLiteral("Twilight Princess"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MobyGamesFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Legend of Zelda: Twilight Princess"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("platform"), QStringLiteral("Nintendo Wii"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Action; Behind view; Puzzle elements; Metroidvania; Fantasy"));
//  QCOMPARE(entry->field("certification"), QStringLiteral("Teen"));
  QCOMPARE(entry->field("pegi"), QStringLiteral("PEGI 12"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Nintendo of America Inc."));
  QCOMPARE(entry->field("developer"), QStringLiteral("Nintendo EAD"));
  QCOMPARE(entry->field("mobygames"), QStringLiteral("http://www.mobygames.com/game/legend-of-zelda-twilight-princess"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
