/***************************************************************************
    Copyright (C) 2009-2011 Robby Stephenson <robby@periapsis.org>
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

#include "discogsfetchertest.h"
#include "../fetch/discogsfetcher.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../images/image.h"

#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( DiscogsFetcherTest )

DiscogsFetcherTest::DiscogsFetcherTest() : AbstractFetcherTest()
    , m_needToWait(false)
    , m_config(QFINDTESTDATA("tellicotest_private.config"), KConfig::SimpleConfig) {
}

void DiscogsFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest_private.config"));
}

void DiscogsFetcherTest::testTitle() {
  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Title,
                                       QStringLiteral("Anywhere But Home"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(results.size() > 0);
  Tellico::Data::EntryPtr entry;  //  results can be randomly ordered, loop until wee find the one we want
  for(int i = 0; i < results.size(); ++i) {
    Tellico::Data::EntryPtr test = results.at(i);
    if(test->field(QStringLiteral("artist")).toLower() == QStringLiteral("evanescence")) {
      entry = test;
      break;
    } else {
      qDebug() << "skipping" << test->title() << test->field(QStringLiteral("artist"));
    }
  }
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Anywhere But Home"));
  QVERIFY(!entry->field(QStringLiteral("artist")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("label")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("genre")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("year")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("track")).isEmpty());

  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(entry->field(QStringLiteral("cover")));
  QVERIFY(!img.isNull());
  m_needToWait = true;
}

void DiscogsFetcherTest::testPerson() {
  // the total test case ends up exceeding the throttle limit so pause for a second
  if(m_needToWait) QTest::qWait(1000);

  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Person,
                                       QStringLiteral("Evanescence"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg, cg.name());

  static_cast<Tellico::Fetch::DiscogsFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Evanescence"));
  QVERIFY(!entry->field(QStringLiteral("title")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("label")).isEmpty());

  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(entry->field(QStringLiteral("cover")));
  QVERIFY(!img.isNull());
  m_needToWait = true;
}

void DiscogsFetcherTest::testKeyword() {
  // the total test case ends up exceeding the throttle limit so pause for a second
  if(m_needToWait) QTest::qWait(1000);

  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QStringLiteral("Fallen Evanescence 2004"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg, cg.name());

  static_cast<Tellico::Fetch::DiscogsFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Fallen"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Evanescence"));
  QVERIFY(!entry->field(QStringLiteral("label")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("year")).isEmpty());

  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(entry->field(QStringLiteral("cover")));
  QVERIFY(!img.isNull());
  m_needToWait = true;
}

// use the Raw query type to fully test the data for a Discogs release
void DiscogsFetcherTest::testRawData() {
  if(m_needToWait) QTest::qWait(1000);

  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Raw,
                                       QStringLiteral("q=r1588789"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Anywhere But Home"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Evanescence"));
  QCOMPARE(entry->field(QStringLiteral("label")), QStringLiteral("Wind-Up"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2004"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Rock"));
  QCOMPARE(entry->field(QStringLiteral("discogs")), QStringLiteral("https://www.discogs.com/Evanescence-Anywhere-But-Home/release/1588789"));
  QCOMPARE(entry->field(QStringLiteral("nationality")), QStringLiteral("Australia & New Zealand"));
  QCOMPARE(entry->field(QStringLiteral("medium")), QStringLiteral("Compact Disc"));

  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.count(), 14);
  QCOMPARE(trackList.at(0), QStringLiteral("Haunted::Evanescence::4:04"));

  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  m_needToWait = true;
}

// do another check to make sure the Vinyl format is captured
void DiscogsFetcherTest::testRawDataVinyl() {
  if(m_needToWait) QTest::qWait(1000);

  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Raw,
                                       QStringLiteral("q=r456552"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("The Clash"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("The Clash"));
  QCOMPARE(entry->field(QStringLiteral("label")), QStringLiteral("CBS; CBS"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1977"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Rock"));
  QCOMPARE(entry->field(QStringLiteral("discogs")), QStringLiteral("https://www.discogs.com/The-Clash-The-Clash/release/456552"));
  QCOMPARE(entry->field(QStringLiteral("nationality")), QStringLiteral("UK"));
  QCOMPARE(entry->field(QStringLiteral("medium")), QStringLiteral("Vinyl"));

  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.count(), 14);
  QCOMPARE(trackList.at(0), QStringLiteral("Janie Jones::The Clash::2:05"));
  m_needToWait = true;
}
