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

#include "collectorztest.h"

#include "../translators/collectorzimporter.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/musiccollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"
#include "../utils/datafileregistry.h"

#include <KLocalizedString>

#include <QTest>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( CollectorzTest )

void CollectorzTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/collectorz2tellico.xsl"));
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  Tellico::ImageFactory::init();
}

void CollectorzTest::testBooks() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/collectorz_books.xml"));
  Tellico::Import::CollectorzImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Book));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 33);

  Tellico::Data::EntryPtr entry = coll->entryById(476);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("1812: The Rivers of War"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("author"), QStringLiteral("Eric Flint"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Del Rey Book"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("0-345-46568-7"));
  QCOMPARE(entry->field("binding"), QStringLiteral("Paperback"));
  QCOMPARE(entry->field("series"), QStringLiteral("Trail of Glory"));
  QCOMPARE(entry->field("series_num"), QStringLiteral("1"));
  QCOMPARE(entry->field("read"), QStringLiteral("true"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Alternate History"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2017-04-29"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("comments").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field("cover").contains(QLatin1Char('/')));
  QVERIFY(!entry->field("cover").contains(QLatin1Char('\\')));
}

void CollectorzTest::testMovies() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/collectorz_movies.xml"));
  Tellico::Import::CollectorzImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Video));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 14);

  // first a movie
  Tellico::Data::EntryPtr entry = coll->entryById(234);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Shaun Of The Dead"));
  QCOMPARE(entry->field("year"), QStringLiteral("2004"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("UK"));
  QCOMPARE(entry->field("language"), QStringLiteral("English"));
  QCOMPARE(entry->field("director"), QStringLiteral("Edgar Wright"));
  QCOMPARE(entry->field("producer"), QStringLiteral("Tim Bevan; Eric Fellner"));
  QCOMPARE(entry->field("writer"), QStringLiteral("Edgar Wright; Simon Pegg"));
  QStringList cast = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QCOMPARE(cast.count(), 10);
  QStringList cast0 = QStringList() << QStringLiteral("Kate Ashfield") << QStringLiteral("Liz");
  QCOMPARE(cast.at(0), cast0.join(Tellico::FieldFormat::columnDelimiterString()));
  QCOMPARE(entry->field("studio"), QStringLiteral("Universal Studios"));
  QCOMPARE(entry->field("medium"), QStringLiteral("DVD"));
  QCOMPARE(entry->field("color"), QStringLiteral("Color"));
  QCOMPARE(entry->field("aspect-ratio"), QStringLiteral("2.35:1"));
  QCOMPARE(entry->field("widescreen"), QStringLiteral("true"));
  QCOMPARE(entry->field("running-time"), QStringLiteral("100"));
  QCOMPARE(entry->field("certification"), QStringLiteral("R (USA)"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Comedy; Horror; Thriller"));
  QCOMPARE(entry->field("subtitle"), QStringLiteral("English; French"));
  QCOMPARE(entry->field("audio-track"), QStringLiteral("030 Dolby Digital 5.1 [English]; 030 Dolby Digital 5.1 [French]; 030 Dolby Digital 5.1 [Spanish]"));
  QCOMPARE(entry->field("region"), QStringLiteral("Region 1"));
  QCOMPARE(entry->field("rating"), QStringLiteral("3"));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("$44.55"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("2012-03-23"));
  QCOMPARE(entry->field("imdb"), QStringLiteral("http://www.imdb.com/title/tt0365748"));
  QVERIFY(!entry->field("plot").isEmpty());
  QCOMPARE(entry->field("mdate"), QStringLiteral("2019-08-25"));
}

void CollectorzTest::testMusic() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/collectorz_music.xml"));
  Tellico::Import::CollectorzImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Album));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 16);

  Tellico::Data::EntryPtr entry = coll->entryById(275);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("A Quiet Normal Life: The Best Of Warren Zevon"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Warren Zevon"));
  QCOMPARE(entry->field("year"), QStringLiteral("1986"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Rock"));
  QCOMPARE(entry->field("medium"), QStringLiteral("Compact Disc"));
  QCOMPARE(entry->field("label"), QStringLiteral("Elektra"));
  QStringList tracks = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(tracks.count(), 14);
  QStringList track1 = QStringList() << QStringLiteral("Werewolves Of London") << QStringLiteral("Warren Zevon") << QStringLiteral("03:28");
  QCOMPARE(tracks.at(0), track1.join(Tellico::FieldFormat::columnDelimiterString()));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("$11.98"));
  QVERIFY(!entry->field("amazon").isEmpty());
  QCOMPARE(entry->field("mdate"), QStringLiteral("2017-04-29"));

  // test conductor
  entry = coll->entryById(192);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Great Performances: Grieg/Schumann Piano Concertos"));
  QCOMPARE(entry->field("artist"), QStringLiteral("The Cleveland Orchestra; Leon Fleisher"));
  QCOMPARE(entry->field("year"), QStringLiteral("2004"));
  QCOMPARE(entry->field("conductor"), QStringLiteral("George Szell"));
}
