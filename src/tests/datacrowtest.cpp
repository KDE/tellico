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

#include "datacrowtest.h"

#include "../translators/datacrowimporter.h"
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

QTEST_GUILESS_MAIN( DataCrowTest )

void DataCrowTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/datacrow2tellico.xsl"));
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  Tellico::ImageFactory::init();
}

void DataCrowTest::testBooks() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/datacrow_books.xml"));
  Tellico::Import::DataCrowImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Book));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("1632"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("2013"));
  QCOMPARE(entry->field("language"), QStringLiteral("English"));
  QCOMPARE(entry->field("edition"), QStringLiteral("First Edition"));
  QCOMPARE(entry->field("author"), QStringLiteral("Eric Flint"));
  QCOMPARE(entry->field("pages"), QStringLiteral("329"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("1-62579-070-8"));
  QCOMPARE(entry->field("binding"), QStringLiteral("Hardback"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Baen Publishing"));
  QCOMPARE(entry->field("series"), QStringLiteral("Ring of Fire"));
  QCOMPARE(entry->field("rating"), QStringLiteral("3"));
  QCOMPARE(entry->field("keyword"), QStringLiteral("favorite"));
  QCOMPARE(entry->field("read"), QStringLiteral("true"));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field("cover").contains(QLatin1Char('/')));
  QVERIFY(!entry->field("plot").isEmpty());
  QCOMPARE(entry->field("cdate"), QStringLiteral("2022-03-10"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2022-03-10"));
}

void DataCrowTest::testBooks5() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/datacrow_books5.xml"));
  Tellico::Import::DataCrowImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Book));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("1632"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("2013"));
  QCOMPARE(entry->field("language"), QStringLiteral("English"));
  QCOMPARE(entry->field("edition"), QStringLiteral("First Edition"));
  QCOMPARE(entry->field("author"), QStringLiteral("Eric Flint"));
  QCOMPARE(entry->field("pages"), QStringLiteral("597"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("1-41653-281-1"));
  QCOMPARE(entry->field("binding"), QStringLiteral("Paperback"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Baen Books"));
  QCOMPARE(entry->field("series"), QStringLiteral("Ring of Fire"));
  QCOMPARE(entry->field("rating"), QStringLiteral("2"));
  QCOMPARE(entry->field("read"), QStringLiteral("true"));
  QVERIFY(!entry->field("plot").isEmpty());
  QCOMPARE(entry->field("cdate"), QStringLiteral("2025-11-06"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2025-11-06"));
}

void DataCrowTest::testMovies() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/datacrow_movies.xml"));
  Tellico::Import::DataCrowImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Video));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Spider-Man: Homecoming"));
  QCOMPARE(entry->field("year"), QStringLiteral("2017"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("United States"));
  QCOMPARE(entry->field("language"), QStringLiteral("English"));
  QCOMPARE(entry->field("subtitle"), QStringLiteral("English"));
  QCOMPARE(entry->field("director"), QStringLiteral("Jon Watts"));
  QStringList cast = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QCOMPARE(cast.count(), 78);
  QCOMPARE(cast.at(0), QStringLiteral("Abraham Attah"));
  QCOMPARE(entry->field("color"), QStringLiteral("Color"));
  QCOMPARE(entry->field("medium"), QStringLiteral("DVD"));
  QCOMPARE(entry->field("aspect-ratio"), QStringLiteral("16:9"));
  QCOMPARE(entry->field("widescreen"), QStringLiteral("true"));
  QCOMPARE(entry->field("running-time"), QStringLiteral("133"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Action; Adventure; Drama; Science Fiction"));
  QCOMPARE(entry->field("rating"), QStringLiteral("5"));
  QCOMPARE(entry->field("keyword"), QStringLiteral("favorite"));
  QCOMPARE(entry->field("seen"), QStringLiteral("true"));
  QCOMPARE(entry->field("url"), QStringLiteral("http://www.spidermanhomecoming.com"));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field("cover").contains(QLatin1Char('/')));
  QVERIFY(!entry->field("plot").isEmpty());
  QCOMPARE(entry->field("cdate"), QStringLiteral("2022-03-10"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2022-03-10"));
}

void DataCrowTest::testMovies5() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/datacrow_movies5.xml"));
  Tellico::Import::DataCrowImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Video));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 3);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Interstellar"));
  QCOMPARE(entry->field("year"), QStringLiteral("2014"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("United Kingdom; United States"));
  QCOMPARE(entry->field("language"), QStringLiteral("English"));
  QCOMPARE(entry->field("subtitle"), QStringLiteral("English"));
  QCOMPARE(entry->field("director"), QStringLiteral("Christopher Nolan"));
  QStringList cast = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QCOMPARE(cast.count(), 34);
  QCOMPARE(cast.at(2), QStringLiteral("Anne Hathaway"));
  QCOMPARE(entry->field("color"), QStringLiteral("Color"));
  QCOMPARE(entry->field("medium"), QStringLiteral("DVD"));
  QCOMPARE(entry->field("aspect-ratio"), QStringLiteral("16:9"));
  QCOMPARE(entry->field("widescreen"), QStringLiteral("true"));
  QCOMPARE(entry->field("running-time"), QStringLiteral("169"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Adventure; Drama; Science Fiction"));
  QCOMPARE(entry->field("rating"), QStringLiteral("4"));
  QCOMPARE(entry->field("keyword"), QStringLiteral("favorite"));
  QCOMPARE(entry->field("seen"), QStringLiteral("true"));
  QCOMPARE(entry->field("url"), QStringLiteral("http://www.interstellarmovie.net/"));
  QVERIFY(!entry->field("plot").isEmpty());
  QCOMPARE(entry->field("cdate"), QStringLiteral("2025-11-04"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2025-11-06"));
}

void DataCrowTest::testMusic() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/datacrow_music.xml"));
  Tellico::Import::DataCrowImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Album));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Love and Thunder"));
  QCOMPARE(entry->field("year"), QStringLiteral("2003"));
  QCOMPARE(entry->field("location"), QStringLiteral("container1"));
  QCOMPARE(entry->field("label"), QStringLiteral("Essential"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Andrew Peterson"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Christian"));
  QCOMPARE(entry->field("rating"), QStringLiteral("4"));
  QStringList tracks = Tellico::FieldFormat::splitTable(entry->field("track"));
  QCOMPARE(tracks.count(), 2);
  QStringList track2{QStringLiteral("Let There Be Light"), QStringLiteral("Andrew Peterson"), QStringLiteral("03:56")};
  QCOMPARE(tracks.at(1), track2.join(Tellico::FieldFormat::columnDelimiterString()));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field("cover").contains(QLatin1Char('/')));
  QCOMPARE(entry->field("cdate"), QStringLiteral("2022-03-11"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2022-03-11"));
}

void DataCrowTest::testMusic5() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/datacrow_music5.xml"));
  Tellico::Import::DataCrowImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Album));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 2);

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Love and Thunder"));
  QCOMPARE(entry->field("year"), QStringLiteral("2003"));
  QCOMPARE(entry->field("medium"), QStringLiteral("Compact Disc"));
  QCOMPARE(entry->field("location"), QStringLiteral("container1"));
  QCOMPARE(entry->field("label"), QStringLiteral("Watershed Records"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Andrew Peterson"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Christian"));
  QCOMPARE(entry->field("rating"), QStringLiteral("4"));
  QStringList tracks = Tellico::FieldFormat::splitTable(entry->field("track"));
  QCOMPARE(tracks.count(), 10);
  QStringList track2{QStringLiteral("Let There Be Light"), QStringLiteral("Andrew Peterson"), QStringLiteral("03:56")};
  QCOMPARE(tracks.at(1), track2.join(Tellico::FieldFormat::columnDelimiterString()));
  QCOMPARE(entry->field("cdate"), QStringLiteral("2025-11-08"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2025-11-08"));
}
