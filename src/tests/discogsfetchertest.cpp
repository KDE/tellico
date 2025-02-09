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
#include "../collections/musiccollection.h"
#include "../images/imagefactory.h"
#include "../images/image.h"

#include <KConfigGroup>
#include <KLocalizedString>

#include <QTest>

QTEST_GUILESS_MAIN( DiscogsFetcherTest )

DiscogsFetcherTest::DiscogsFetcherTest() : AbstractFetcherTest()
    , m_needToWait(false) {
}

void DiscogsFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest_private.config"));
  if(m_hasConfigFile) {
    m_config = KSharedConfig::openConfig(QFINDTESTDATA("tellicotest_private.config"), KConfig::SimpleConfig);
  }
}

void DiscogsFetcherTest::cleanup() {
  m_needToWait = true;
}

void DiscogsFetcherTest::testTitle() {
  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Title,
                                       QStringLiteral("Anywhere But Home"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(results.size() > 0);
  Tellico::Data::EntryPtr entry;  //  results can be randomly ordered, loop until we find the one we want
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
}

void DiscogsFetcherTest::testPerson() {
  // the total test case ends up exceeding the throttle limit so pause for a second
  if(m_needToWait) QTest::qWait(5000);

  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Person,
                                       QStringLiteral("Evanescence"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

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
}

void DiscogsFetcherTest::testKeyword() {
  // the total test case ends up exceeding the throttle limit so pause for a second
  if(m_needToWait) QTest::qWait(5000);

  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QStringLiteral("Fallen Evanescence 2004"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

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
}

void DiscogsFetcherTest::testBarcode() {
  // the total test case ends up exceeding the throttle limit so pause for a second
  if(m_needToWait) QTest::qWait(5000);

  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::UPC,
                                       QStringLiteral("4 547366 014099"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

  static_cast<Tellico::Fetch::DiscogsFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Fallen"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Evanescence"));
  QVERIFY(!entry->field(QStringLiteral("label")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("year")).isEmpty());
  QCOMPARE(entry->field(QStringLiteral("barcode")), QStringLiteral("4 547366 014099"));

  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(entry->field(QStringLiteral("cover")));
  QVERIFY(!img.isNull());
}

// use the Raw query type to fully test the data for a Discogs release
void DiscogsFetcherTest::testRawData() {
  if(m_needToWait) QTest::qWait(5000);

  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Raw,
                                       QStringLiteral("q=r1588789"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg);

  static_cast<Tellico::Fetch::DiscogsFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry->field(QStringLiteral("title")).contains(QStringLiteral("Anywhere But Home")));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Evanescence"));
  QCOMPARE(entry->field(QStringLiteral("label")), QStringLiteral("Wind-Up"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2004"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Rock"));
  QCOMPARE(entry->field(QStringLiteral("discogs")), QStringLiteral("https://www.discogs.com/release/1588789-Evanescence-Anywhere-But-Home"));
  QCOMPARE(entry->field(QStringLiteral("nationality")), QStringLiteral("Australia & New Zealand"));
  QCOMPARE(entry->field(QStringLiteral("medium")), QStringLiteral("Compact Disc"));
  QCOMPARE(entry->field(QStringLiteral("catno")), QStringLiteral("5192073000"));

  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.count(), 14);
  QCOMPARE(trackList.at(0), QStringLiteral("Haunted::Evanescence::4:04"));

  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(entry->field(QStringLiteral("cover")));
  QVERIFY(!img.isNull());
}

// do another check to make sure the Vinyl format is captured
void DiscogsFetcherTest::testRawDataVinyl() {
  if(m_needToWait) QTest::qWait(5000);

  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Raw,
                                       QStringLiteral("q=r456552"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg);

  static_cast<Tellico::Fetch::DiscogsFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry->field(QStringLiteral("title")).contains(QStringLiteral("The Clash")));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("The Clash"));
  QCOMPARE(entry->field(QStringLiteral("label")), QStringLiteral("CBS; CBS"));
//  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1977"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Rock"));
  QCOMPARE(entry->field(QStringLiteral("discogs")), QStringLiteral("https://www.discogs.com/release/456552-The-Clash-The-Clash"));
  QCOMPARE(entry->field(QStringLiteral("nationality")), QStringLiteral("UK"));
  QCOMPARE(entry->field(QStringLiteral("medium")), QStringLiteral("Vinyl"));

  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.count(), 14);
  QCOMPARE(trackList.at(0), QStringLiteral("Janie Jones::The Clash::2:05"));
}

void DiscogsFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::MusicCollection(true));
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField(QStringLiteral("title"), QStringLiteral("Nevermind"));
  entry->setField(QStringLiteral("artist"), QStringLiteral("Nirvana"));
  entry->setField(QStringLiteral("year"), QStringLiteral("1991"));
  coll->addEntries(entry);

  Tellico::Fetch::DiscogsFetcher fetcher(this);
  auto request = fetcher.updateRequest(entry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QVERIFY(request.value().contains(QStringLiteral("title=Nevermind")));
  QVERIFY(request.value().contains(QStringLiteral("artist=Nirvana")));
  QVERIFY(request.value().contains(QStringLiteral("year=1991")));
}

// bug 479503, https://bugs.kde.org/show_bug.cgi?id=479503
void DiscogsFetcherTest::testMultiDisc() {
  // the total test case ends up exceeding the throttle limit so pause for a bit
  if(m_needToWait) QTest::qWait(5000);

  QString groupName = QStringLiteral("Discogs");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::UPC,
                                       QStringLiteral("4988031446843"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->title(), QStringLiteral("Silver Lining Suite"));
  auto trackField = entry->collection()->fieldByName(QStringLiteral("track"));
  QVERIFY(trackField);
  // verify the title was updated to include the disc number
  QVERIFY(trackField->title() != i18n("Tracks"));
  QStringList tracks1 = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(tracks1.count(), 9);
  QCOMPARE(tracks1.first(), QStringLiteral("Isolation::Hiromi Uehara::"));
  // new field with Disc 2 tracks
  QStringList tracks2 = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track2")));
  QCOMPARE(tracks2.count(), 8);
  QCOMPARE(tracks2.first(), QStringLiteral("Somewhere::Hiromi Uehara::"));
}

// https://bugs.kde.org/show_bug.cgi?id=499401
void DiscogsFetcherTest::testMultiDiscOldWay() {
  // the total test case ends up exceeding the throttle limit so pause for a bit
  if(m_needToWait) QTest::qWait(5000);

  // group 2 has config to use old single track approach
  QString groupName = QStringLiteral("Discogs2");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::UPC,
                                       QStringLiteral("4988031446843"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->title(), QStringLiteral("Silver Lining Suite"));
  auto trackField = entry->collection()->fieldByName(QStringLiteral("track"));
  QVERIFY(trackField);
  QVERIFY(!entry->collection()->hasField(QStringLiteral("track2")));
  // verify the title was not updated to include the disc number
  QVERIFY(trackField->title() == i18n("Tracks"));
  QStringList tracks = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(tracks.count(), 17);
  QCOMPARE(tracks.at(0), QStringLiteral("Isolation::Hiromi Uehara::"));
  QCOMPARE(tracks.at(8), QStringLiteral("Ribera Del Duero::Hiromi Uehara::"));
  QCOMPARE(tracks.at(9), QStringLiteral("Somewhere::Hiromi Uehara::"));
  QCOMPARE(tracks.at(16), QStringLiteral("Sepia Effect::Hiromi Uehara::"));
}
