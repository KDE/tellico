/***************************************************************************
    Copyright (C) 2019-2020 Robby Stephenson <robby@periapsis.org>
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

#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( MobyGamesFetcherTest )

MobyGamesFetcherTest::MobyGamesFetcherTest() : AbstractFetcherTest(), m_needToWait(false) {
}

void MobyGamesFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest_private.config"));
  if(m_hasConfigFile) {
    m_config = KSharedConfig::openConfig(QFINDTESTDATA("tellicotest_private.config"), KConfig::SimpleConfig);
  }
}

void MobyGamesFetcherTest::init() {
  if(m_needToWait) QTest::qSleep(1000);
  m_needToWait = false;
}

void MobyGamesFetcherTest::cleanup() {
  m_needToWait = true;
}

void MobyGamesFetcherTest::testTitle() {
  const QString groupName = QStringLiteral("MobyGames");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with MobyGames settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Title,
                                       QStringLiteral("Twilight Princess"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MobyGamesFetcher(this));
  fetcher->readConfig(cg);

  // since the platforms are read in the next event loop (single shot timer)
  // to avoid downloading again, wait a moment
  qApp->processEvents();

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 5);
  Tellico::Data::EntryPtr entry;
  for(const auto& testEntry : std::as_const(results)) {
    if(testEntry->field(QStringLiteral("platform")) == QLatin1String("Nintendo Wii")) {
      entry = testEntry;
      break;
    } else {
      qDebug() << "...skipping" << testEntry->title();
    }
  }
  QVERIFY(entry);

  QCOMPARE(entry->field("title"), QStringLiteral("The Legend of Zelda: Twilight Princess"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("platform"), QStringLiteral("Nintendo Wii"));
  QVERIFY(entry->field("genre").contains(QStringLiteral("Action")));
//  QCOMPARE(entry->field("certification"), QStringLiteral("Teen"));
  QCOMPARE(entry->field("pegi"), QStringLiteral("PEGI 12"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Nintendo Co., Ltd."));
  QCOMPARE(entry->field("developer"), QStringLiteral("Nintendo EAD"));
  QCOMPARE(entry->field("mobygames"), QStringLiteral("https://www.mobygames.com/game/25103/the-legend-of-zelda-twilight-princess/"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("screenshot")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("screenshot")).contains(QLatin1Char('/')));
}

// same search, except to include platform name in search
// which is a Keyword search and the WiiU result should be first
void MobyGamesFetcherTest::testKeyword() {
  const QString groupName = QStringLiteral("MobyGames");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with MobyGames settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Keyword,
                                       QStringLiteral("Twilight Princess Nintendo WiiU"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MobyGamesFetcher(this));
  fetcher->readConfig(cg);

  // since the platforms are read in the next event loop (single shot timer)
  // to avoid downloading again, wait a moment
  qApp->processEvents();

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Legend of Zelda: Twilight Princess"));
  QCOMPARE(entry->field("year"), QStringLiteral("2016"));
  QCOMPARE(entry->field("platform"), QStringLiteral("Nintendo WiiU"));
}

void MobyGamesFetcherTest::testRaw() {
  // MobyGames2 group has no image
  const QString groupName = QStringLiteral("MobyGames2");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with MobyGames settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Raw,
                                       QStringLiteral("id=25103&platform=82"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MobyGamesFetcher(this));
  fetcher->readConfig(cg);

  // since the platforms are read in the next event loop (single shot timer)
  // to avoid downloading again, wait a moment
  qApp->processEvents();

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Legend of Zelda: Twilight Princess"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("platform"), QStringLiteral("Nintendo Wii"));
  QVERIFY(entry->field("genre").contains(QStringLiteral("Action")));
  QCOMPARE(entry->field("certification"), QStringLiteral("Teen"));
  QCOMPARE(entry->field("pegi"), QStringLiteral("PEGI 12"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Nintendo Co., Ltd."));
  QCOMPARE(entry->field("developer"), QStringLiteral("Nintendo EAD"));
  QCOMPARE(entry->field("mobygames"), QStringLiteral("https://www.mobygames.com/game/25103/the-legend-of-zelda-twilight-princess/"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  // no cover image downloaded
  QVERIFY(entry->field(QStringLiteral("cover")).isEmpty());
}

void MobyGamesFetcherTest::testUpdateRequest() {
  // MobyGames2 group has no image
  const QString groupName = QStringLiteral("MobyGames2");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with MobyGames settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::MobyGamesFetcher fetcher(this);
  fetcher.readConfig(cg);

  // since the platforms are read in the next event loop (single shot timer)
  // to avoid downloading again, wait a moment
  qApp->processEvents();

  // create an entry and check the update request
  Tellico::Data::CollPtr coll(new Tellico::Data::GameCollection(true));
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField(QStringLiteral("title"), QStringLiteral("T"));

  Tellico::Fetch::FetchRequest req = fetcher.updateRequest(entry);
  QCOMPARE(req.key(), Tellico::Fetch::Title);
  QCOMPARE(req.value(), entry->title());

  // test having a user customized platform
  QString p(QStringLiteral("playstation 4")); // pId = 141
  Tellico::Data::FieldPtr f = coll->fieldByName(QStringLiteral("platform"));
  QVERIFY(f);
  if(!f->allowed().contains(p)) {
    f->setAllowed(QStringList(f->allowed()) << p);
  }

  entry->setField(QStringLiteral("platform"), p);
  req = fetcher.updateRequest(entry);
  QCOMPARE(req.key(), Tellico::Fetch::Raw);
  QCOMPARE(req.value(), QStringLiteral("title=T&platform=141"));

  // test having an unknown platform
  p = QStringLiteral("Atari 2600"); // pId = 28
  if(!f->allowed().contains(p)) {
    f->setAllowed(QStringList(f->allowed()) << p);
  }

  entry->setField(QStringLiteral("platform"), p);
  req = fetcher.updateRequest(entry);
  QCOMPARE(req.key(), Tellico::Fetch::Raw);
  QCOMPARE(req.value(), QStringLiteral("title=T&platform=28"));
}
