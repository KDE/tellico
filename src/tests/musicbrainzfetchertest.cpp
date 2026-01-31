/***************************************************************************
    Copyright (C) 2010-2011 Robby Stephenson <robby@periapsis.org>
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

#include "musicbrainzfetchertest.h"

#include "../fetch/musicbrainzfetcher.h"
#include "../fetch/fetcherinitializer.h"
#include "../collections/musiccollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KSharedConfig>
#include <KLocalizedString>

#include <QTest>

QTEST_GUILESS_MAIN( MusicBrainzFetcherTest )

MusicBrainzFetcherTest::MusicBrainzFetcherTest() : AbstractFetcherTest() {
}

void MusicBrainzFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  Tellico::Fetch::RegisterFetcher<Tellico::Fetch::MusicBrainzFetcher> registerMB(Tellico::Fetch::MusicBrainz);

  // since we use the importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/musicbrainz2tellico.xsl"));
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("carried along"));
  m_fieldValues.insert(QStringLiteral("artist"), QStringLiteral("Andrew Peterson"));
  m_fieldValues.insert(QStringLiteral("label"), QStringLiteral("essential records"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("2000"));
  m_fieldValues.insert(QStringLiteral("medium"), QStringLiteral("compact disc"));
}

void MusicBrainzFetcherTest::cleanup() {
  // the total test case can end up exceeding the throttle limit so pause for a second
  QTest::qWait(1000);
}

void MusicBrainzFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Title,
                                       m_fieldValues.value(QStringLiteral("title")));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  static_cast<Tellico::Fetch::MusicBrainzFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QStringLiteral("track")).isEmpty());
  // no link
  QVERIFY(entry->field(QStringLiteral("musicbrainz")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void MusicBrainzFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       m_fieldValues.value(QStringLiteral("title")));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));

  static_cast<Tellico::Fetch::MusicBrainzFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QStringLiteral("track")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void MusicBrainzFetcherTest::testBug426560() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QStringLiteral("lily allen - no shame"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  static_cast<Tellico::Fetch::MusicBrainzFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->title(), QStringLiteral("No Shame"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Lily Allen"));
}

void MusicBrainzFetcherTest::testPerson() {
  const QString artist(QStringLiteral("artist"));
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Person,
                                       m_fieldValues.value(artist));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));

  // TODO: Fetcher::setLimit should be virtual in Fetcher class
  static_cast<Tellico::Fetch::MusicBrainzFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QVERIFY(results.size() > 0);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(artist), m_fieldValues.value(artist));
}

void MusicBrainzFetcherTest::testACDC() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Person,
                                       QStringLiteral("AC/DC"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  // TODO: Fetcher::setLimit should be virtual in Fetcher class
  static_cast<Tellico::Fetch::MusicBrainzFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QVERIFY(results.size() > 0);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("AC/DC"));
}

// test grabbing cover art from coverartarchive.org
void MusicBrainzFetcherTest::testCoverArt() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Title,
                                       QStringLiteral("Laulut ja tarinat"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));

  static_cast<Tellico::Fetch::MusicBrainzFetcher*>(fetcher.data())->setLimit(1);
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->title(), QStringLiteral("Laulut Ja Tarinat"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void MusicBrainzFetcherTest::testSoundtrack() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Title,
                                       QStringLiteral("legend of bagger vance"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->title(), QStringLiteral("Legend of Bagger Vance, The"));
  // sound tracks are the only genre tag that is read
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Soundtrack"));
}

void MusicBrainzFetcherTest::testBarcode() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("musicbrainz"));
  cg.writeEntry("Custom Fields", QStringLiteral("barcode,musicbrainz"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::UPC,
                                       QStringLiteral("8024391054123"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->title(), QStringLiteral("Old Man and the Spirit, The"));
  QCOMPARE(entry->field(QStringLiteral("barcode")), QStringLiteral("8024391054123"));
  QCOMPARE(entry->field(QStringLiteral("musicbrainz")), QStringLiteral("https://musicbrainz.org/release/13e56796-7e90-48fb-9acf-72ef590aa2ce"));
}

void MusicBrainzFetcherTest::testCatno() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("musicbrainz"));
  cg.writeEntry("Custom Fields", QStringLiteral("barcode,musicbrainz"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QStringLiteral("catno:\"82876 52008 2\""));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->title(), QStringLiteral("Ultimate Dolly Parton"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Dolly Parton"));
  QCOMPARE(entry->field(QStringLiteral("musicbrainz")), QStringLiteral("https://musicbrainz.org/release/9f848849-2e1b-4808-982d-acd18ee34bc5"));
}

void MusicBrainzFetcherTest::testMbid() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("musicbrainz"));
  cg.writeEntry("Custom Fields", QStringLiteral("barcode,musicbrainz"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QStringLiteral("mbid:9f848849-2e1b-4808-982d-acd18ee34bc5"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->title(), QStringLiteral("Ultimate Dolly Parton"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Dolly Parton"));
  QCOMPARE(entry->field(QStringLiteral("musicbrainz")), QStringLiteral("https://musicbrainz.org/release/9f848849-2e1b-4808-982d-acd18ee34bc5"));
}

// bug 479503, https://bugs.kde.org/show_bug.cgi?id=479503
void MusicBrainzFetcherTest::testMultiDisc() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::UPC,
                                       QStringLiteral("4988031446843"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));

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
  // new field with Disc 2 tracks
  QStringList tracks2 = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track2")));
  QCOMPARE(tracks2.count(), 8);
  QCOMPARE(tracks2.first(), QStringLiteral("Somewhere::上原ひろみ::7:31"));
}

// https://bugs.kde.org/show_bug.cgi?id=499401
void MusicBrainzFetcherTest::testMultiDiscOldWay() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("musicbrainz"));
  cg.writeEntry("Split Tracks By Disc", false);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::UPC,
                                       QStringLiteral("4988031446843"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MusicBrainzFetcher(this));
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
  QCOMPARE(tracks.at(8), QStringLiteral("Ribera Del Duero::上原ひろみ::4:00"));
  QCOMPARE(tracks.at(9), QStringLiteral("Somewhere::上原ひろみ::7:31"));
  QCOMPARE(tracks.at(16), QStringLiteral("Sepia Effect::上原ひろみ::7:38"));
}

void MusicBrainzFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::MusicCollection(true));
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("musicbrainz"),
                                                         QStringLiteral("musicbrainz"),
                                                         Tellico::Data::Field::URL));
  QCOMPARE(coll->addField(field), true);
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);
  entry->setField(QStringLiteral("title"), QStringLiteral("Ultimate Dolly Parton"));
  entry->setField(QStringLiteral("artist"), QStringLiteral("Dolly Parton"));

  Tellico::Fetch::MusicBrainzFetcher fetcher(this);
  auto request = fetcher.updateRequest(entry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QCOMPARE(request.value(), QStringLiteral("release:\"Ultimate Dolly Parton\" AND artist:\"Dolly Parton\""));

  entry->setField(QStringLiteral("musicbrainz"), QStringLiteral("https://musicbrainz.org/release/9f848849-2e1b-4808-982d-acd18ee34bc5"));
  request = fetcher.updateRequest(entry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QCOMPARE(request.value(), QStringLiteral("reid:9f848849-2e1b-4808-982d-acd18ee34bc5"));
}
