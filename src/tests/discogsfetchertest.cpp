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
#include "qtest_kde.h"

#include "../fetch/discogsfetcher.h"
#include "../collections/musiccollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../images/image.h"

#include <KStandardDirs>
#include <KConfigGroup>

QTEST_KDEMAIN( DiscogsFetcherTest, GUI )

DiscogsFetcherTest::DiscogsFetcherTest() : AbstractFetcherTest()
    , m_config(QString::fromLatin1(KDESRCDIR)  + "/discogsfetchertest.config", KConfig::SimpleConfig) {
}

void DiscogsFetcherTest::initTestCase() {
//  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  // since we use the Discogs importer
//  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();
  m_hasConfigFile = QFile::exists(QString::fromLatin1(KDESRCDIR)  + "/discogsfetchertest.config");
}

void DiscogsFetcherTest::testTitle() {
  QString groupName = QLatin1String("Discogs");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Title,
                                       QLatin1String("Anywhere But Home"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(results.size() > 0);
  Tellico::Data::EntryPtr entry;  //  results can be randomly ordered, loop until wee find the one we want
  for(int i = 0; i < results.size(); ++i) {
    Tellico::Data::EntryPtr test = results.at(i);
    if(test->field(QLatin1String("artist")).toLower() == QLatin1String("evanescence")) {
      entry = test;
      break;
    } else {
      qDebug() << "skipping" << test->title() << test->field(QLatin1String("artist"));
    }
  }
  QVERIFY(entry);

  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Anywhere But Home"));
  QVERIFY(!entry->field(QLatin1String("artist")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("label")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("genre")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("year")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("track")).isEmpty());

  //OAuth is now required
  /*
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(entry->field(QLatin1String("cover")));
  QVERIFY(!img.isNull());
*/
}

void DiscogsFetcherTest::testPerson() {
  QString groupName = QLatin1String("Discogs");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Person,
                                       QLatin1String("Evanescence"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("artist")), QLatin1String("Evanescence"));
  QVERIFY(!entry->field(QLatin1String("title")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("label")).isEmpty());
  //OAuth is now required
  /*
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(entry->field(QLatin1String("cover")));
  QVERIFY(!img.isNull());
  */
}

void DiscogsFetcherTest::testKeyword() {
  QString groupName = QLatin1String("Discogs");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QLatin1String("Fallen Evanescence 2004"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Fallen"));
  QCOMPARE(entry->field(QLatin1String("artist")), QLatin1String("Evanescence"));
  QVERIFY(!entry->field(QLatin1String("label")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("year")).isEmpty());
  // OAuth is now required
  /*
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(entry->field(QLatin1String("cover")));
  QVERIFY(!img.isNull());
  */
}

// use the Raw query type to fully test the data for a Discogs release
void DiscogsFetcherTest::testRawData() {
  QString groupName = QLatin1String("Discogs");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Raw,
                                       QLatin1String("q=1588789"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Anywhere But Home"));
  QCOMPARE(entry->field(QLatin1String("artist")), QLatin1String("Evanescence"));
  QCOMPARE(entry->field(QLatin1String("label")), QLatin1String("Wind-Up"));
  QCOMPARE(entry->field(QLatin1String("year")), QLatin1String("2004"));
  QCOMPARE(entry->field(QLatin1String("genre")), QLatin1String("Rock"));
  QCOMPARE(entry->field(QLatin1String("discogs")), QLatin1String("https://www.discogs.com/Evanescence-Anywhere-But-Home/release/1588789"));
  QCOMPARE(entry->field(QLatin1String("nationality")), QLatin1String("Australia"));
  QCOMPARE(entry->field(QLatin1String("medium")), QLatin1String("Compact Disc"));

  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field("track"));
  QCOMPARE(trackList.count(), 14);
  QCOMPARE(trackList.at(0), QLatin1String("Haunted::Evanescence::4:04"));

  // OAuth is now required
  /*
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(entry->field(QLatin1String("cover")));
  QVERIFY(!img.isNull());
*/
}

// do another check to make sure the Vinyl format is captured
void DiscogsFetcherTest::testRawDataVinyl() {
  QString groupName = QLatin1String("Discogs");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Discogs settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Raw,
                                       QLatin1String("q=r456552"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("The Clash"));
  QCOMPARE(entry->field(QLatin1String("artist")), QLatin1String("Clash, The"));
  QCOMPARE(entry->field(QLatin1String("label")), QLatin1String("CBS; CBS"));
  QCOMPARE(entry->field(QLatin1String("year")), QLatin1String("1977"));
  QCOMPARE(entry->field(QLatin1String("genre")), QLatin1String("Rock"));
  QCOMPARE(entry->field(QLatin1String("discogs")), QLatin1String("https://www.discogs.com/Clash-The-Clash/release/456552"));
  QCOMPARE(entry->field(QLatin1String("nationality")), QLatin1String("UK"));
  QCOMPARE(entry->field(QLatin1String("medium")), QLatin1String("Vinyl"));

  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field("track"));
  QCOMPARE(trackList.count(), 14);
  QCOMPARE(trackList.at(0), QLatin1String("Janie Jones::Clash, The::2:05"));
}
