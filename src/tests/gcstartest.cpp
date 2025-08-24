/***************************************************************************
    Copyright (C) 2009-2010 Robby Stephenson <robby@periapsis.org>
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

#include "gcstartest.h"

#include "../translators/gcstarimporter.h"
#include "../translators/gcstarexporter.h"
#include "../collections/collectioninitializer.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"
#include "../fieldformat.h"

#include <KLocalizedString>

#include <QTest>
#include <QStandardPaths>

#define FIELDS(entry, fieldName) Tellico::FieldFormat::splitValue(entry->field(QStringLiteral(fieldName)))
#define TABLES(entry, fieldName) Tellico::FieldFormat::splitTable(entry->field(QStringLiteral(fieldName)))

QTEST_GUILESS_MAIN( GCstarTest )

void GCstarTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  // remove the test image directory
  QDir gcstarImageDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/gcstar/"));
  gcstarImageDir.removeRecursively();
  Tellico::ImageFactory::init();
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/gcstar2tellico.xsl"));
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("data/Knives.gcm")); // for custom collection
  // need to register the collection types
  Tellico::CollectionInitializer ci;
}

void GCstarTest::testBook() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-book.gcs"));
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 2);
  // should be translated somehow
  QCOMPARE(coll->title(), QStringLiteral("GCstar Import"));
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Reason for God"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("2008"));
  QCOMPARE(FIELDS(entry, "author").count(), 2);
  QCOMPARE(FIELDS(entry, "author").first(), QStringLiteral("Timothy Keller"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("978-0-525-95049-3"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Dutton Adult"));
  QCOMPARE(FIELDS(entry, "genre").count(), 2);
  QCOMPARE(FIELDS(entry, "genre").at(0), QStringLiteral("non-fiction"));
  QCOMPARE(FIELDS(entry, "keyword").count(), 2);
  QCOMPARE(FIELDS(entry, "keyword").at(0), QStringLiteral("tag1"));
  QCOMPARE(FIELDS(entry, "keyword").at(1), QStringLiteral("tag2"));
  // file has rating of 4, Tellico uses half the rating of GCstar, so it should be 2
  QCOMPARE(entry->field("rating"), QStringLiteral("2"));
  QCOMPARE(FIELDS(entry, "language").count(), 1);
  QCOMPARE(FIELDS(entry, "language").at(0), QStringLiteral("English"));

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());

  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());
  QCOMPARE(coll2->title(), coll->title());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // skip images
      if(f->type() != Tellico::Data::Field::Image) {
        QCOMPARE(f->name() + e2->field(f), f->name() + e1->field(f));
      }
    }
  }
}

void GCstarTest::testComicBook() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-comicbook.gcs"));
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::ComicBook);
  QCOMPARE(coll->entryCount(), 1);
  // should be translated somehow
  QCOMPARE(coll->title(), QStringLiteral("GCstar Import"));
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("title"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("2010"));
  QCOMPARE(entry->field("series"), QStringLiteral("series"));
  QCOMPARE(entry->field("issue"), QStringLiteral("1"));
  QCOMPARE(FIELDS(entry, "writer").count(), 2);
  QCOMPARE(FIELDS(entry, "writer").first(), QStringLiteral("writer1"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("1-23456-789-X"));
  QCOMPARE(entry->field("artist"), QStringLiteral("illustrator"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("publisher"));
  QCOMPARE(entry->field("colorist"), QStringLiteral("colourist"));
  QCOMPARE(entry->field("category"), QStringLiteral("category"));
  QCOMPARE(entry->field("format"), QStringLiteral("format"));
  QCOMPARE(entry->field("collection"), QStringLiteral("collection"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("29/08/2010"));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("12.99"));
  QCOMPARE(entry->field("numberboards"), QStringLiteral("1"));
  QCOMPARE(entry->field("signed"), QStringLiteral("true"));
  // file has rating of 4, Tellico uses half the rating of GCstar, so it should be 2
  QCOMPARE(entry->field("rating"), QStringLiteral("2"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("comments").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());

  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());
  QCOMPARE(coll2->title(), coll->title());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // skip images
      if(f->type() != Tellico::Data::Field::Image) {
        QCOMPARE(f->name() + e2->field(f), f->name() + e1->field(f));
      }
    }
  }
}

void GCstarTest::testVideo() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-video.gcs"));
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 3);
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Man from Snowy River"));
  QCOMPARE(entry->field("year"), QStringLiteral("1982"));
  QCOMPARE(FIELDS(entry, "director").count(), 1);
  QCOMPARE(FIELDS(entry, "director").first(), QStringLiteral("George Miller"));
  QCOMPARE(FIELDS(entry, "nationality").count(), 1);
  QCOMPARE(FIELDS(entry, "nationality").first(), QStringLiteral("Australia"));
  QCOMPARE(entry->field("medium"), QStringLiteral("DVD"));
  QCOMPARE(entry->field("running-time"), QStringLiteral("102"));
  QCOMPARE(FIELDS(entry, "genre").count(), 4);
  QCOMPARE(FIELDS(entry, "genre").at(0), QStringLiteral("Drama"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QCOMPARE(castList.count(), 10);
  QCOMPARE(castList.at(0), QStringLiteral("Tom Burlinson::Jim Craig"));
  QCOMPARE(castList.at(2), QStringLiteral("Kirk Douglas::Harrison / Spur"));
  QCOMPARE(FIELDS(entry, "keyword").count(), 2);
  QCOMPARE(FIELDS(entry, "keyword").at(0), QStringLiteral("tag2"));
  QCOMPARE(FIELDS(entry, "keyword").at(1), QStringLiteral("tag1"));
  QCOMPARE(entry->field("rating"), QStringLiteral("3"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("comments").isEmpty());

  entry = coll->entryById(4);
  QVERIFY(entry);
  castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QCOMPARE(castList.count(), 11);
  QCOMPARE(castList.at(0), QStringLiteral("Famke Janssen::Marnie Watson"));
  QCOMPARE(entry->field("location"), QStringLiteral("On Hard Drive"));

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());
  QCOMPARE(coll2->title(), coll->title());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // skip images and tables
      if(f->type() != Tellico::Data::Field::Image &&
         f->type() != Tellico::Data::Field::Table) {
        QCOMPARE(f->name() + e2->field(f), f->name() + e1->field(f));
      } else if(f->type() == Tellico::Data::Field::Table) {
        QStringList rows1 = Tellico::FieldFormat::splitTable(e1->field(f));
        QStringList rows2 = Tellico::FieldFormat::splitTable(e2->field(f));
        QCOMPARE(rows1.count(), rows2.count());
        for(int i = 0; i < rows1.count(); ++i) {
          QCOMPARE(f->name() + rows2.at(i), f->name() + rows1.at(i));
        }
      }
    }
  }
}

void GCstarTest::testMusic() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-music.gcs"));
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 1);
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Lifesong"));
  QCOMPARE(entry->field("year"), QStringLiteral("2005"));
  QCOMPARE(FIELDS(entry, "artist").count(), 1);
  QCOMPARE(FIELDS(entry, "artist").first(), QStringLiteral("Casting Crowns"));
  QCOMPARE(FIELDS(entry, "label").count(), 1);
  QCOMPARE(FIELDS(entry, "label").first(), QStringLiteral("Beach Street Records"));
  QCOMPARE(entry->field("medium"), QStringLiteral("Compact Disc"));
  QCOMPARE(FIELDS(entry, "genre").count(), 2);
  QCOMPARE(FIELDS(entry, "genre").at(0), QStringLiteral("Electronic"));
  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.count(), 11);
  QCOMPARE(trackList.at(1), QStringLiteral("Praise You In This Storm::Casting Crowns::4:59"));
  QCOMPARE(FIELDS(entry, "producer").count(), 1);
  QCOMPARE(FIELDS(entry, "producer").at(0), QStringLiteral("Mark A. Miller"));
  QCOMPARE(FIELDS(entry, "composer").count(), 4);
  QCOMPARE(FIELDS(entry, "composer").at(1), QStringLiteral("David Hunt"));
  QCOMPARE(entry->field("cdate"), QStringLiteral("2009-09-22"));

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());
  QCOMPARE(coll2->title(), coll->title());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // skip images
      if(f->type() != Tellico::Data::Field::Image) {
        QCOMPARE(f->name() + e2->field(f), f->name() + e1->field(f));
      }
    }
  }
}

void GCstarTest::testVideoGame() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-videogame.gcs"));
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Game);
  QCOMPARE(coll->entryCount(), 2);
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Halo 3"));
  QCOMPARE(entry->field("year"), QStringLiteral("2007"));
  QCOMPARE(entry->field("platform"), QStringLiteral("Xbox 360"));
  QCOMPARE(FIELDS(entry, "developer").count(), 1);
  QCOMPARE(FIELDS(entry, "developer").first(), QStringLiteral("Bungie Studios"));
  QCOMPARE(FIELDS(entry, "publisher").count(), 1);
  QCOMPARE(FIELDS(entry, "publisher").first(), QStringLiteral("Microsoft Games Studios"));
  QCOMPARE(FIELDS(entry, "genre").count(), 3);
  QCOMPARE(FIELDS(entry, "genre").at(0), QStringLiteral("Action"));
  QCOMPARE(entry->field("cdate"), QStringLiteral("2009-09-24"));
  QVERIFY(!entry->field("description").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());
  QCOMPARE(coll2->title(), coll->title());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // skip images
      if(f->type() != Tellico::Data::Field::Image) {
        QCOMPARE(f->name() + e2->field(f), f->name() + e1->field(f));
      }
    }
  }
}

void GCstarTest::testBoardGame() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-boardgame.gcs"));
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::BoardGame);
  QCOMPARE(coll->entryCount(), 2);
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Risk"));
  QCOMPARE(entry->field("year"), QStringLiteral("1959"));
  QCOMPARE(FIELDS(entry, "designer").count(), 2);
  QCOMPARE(FIELDS(entry, "designer").at(1), QStringLiteral("Michael I. Levin"));
  QCOMPARE(FIELDS(entry, "publisher").count(), 11);
  QCOMPARE(FIELDS(entry, "publisher").at(1), QStringLiteral("Borras Plana S.A."));
  QCOMPARE(FIELDS(entry, "mechanism").count(), 3);
  QCOMPARE(FIELDS(entry, "mechanism").at(1), QStringLiteral("Dice Rolling"));
  QCOMPARE(FIELDS(entry, "genre").count(), 1);
  QCOMPARE(FIELDS(entry, "genre").at(0), QStringLiteral("Wargame"));
  QVERIFY(!entry->field("description").isEmpty());
  QVERIFY(!entry->field("comments").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());
  QCOMPARE(coll2->title(), coll->title());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // skip images
      if(f->type() != Tellico::Data::Field::Image) {
        QCOMPARE(f->name() + e2->field(f), f->name() + e1->field(f));
      }
    }
  }
}

void GCstarTest::testWine() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-wine.gcs"));
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Wine);
  QCOMPARE(coll->entryCount(), 1);
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("vintage"), QStringLiteral("1990"));
  QCOMPARE(entry->field("producer"), QStringLiteral("producer"));
  QCOMPARE(entry->field("type"), QStringLiteral("Red Wine"));
  QCOMPARE(entry->field("country"), QStringLiteral("australia"));
  QCOMPARE(entry->field("quantity"), QStringLiteral("1"));
  QCOMPARE(FIELDS(entry, "varietal").count(), 2);
  QCOMPARE(FIELDS(entry, "varietal").at(1), QStringLiteral("grape2"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("28/08/2010"));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("12.99"));
  QCOMPARE(entry->field("appellation"), QStringLiteral("designation"));
  QCOMPARE(entry->field("distinction"), QStringLiteral("distinction"));
  QCOMPARE(entry->field("soil"), QStringLiteral("soil"));
  QCOMPARE(entry->field("alcohol"), QStringLiteral("12"));
  QCOMPARE(entry->field("volume"), QStringLiteral("750"));
  QCOMPARE(entry->field("rating"), QStringLiteral("3"));
  QCOMPARE(entry->field("gift"), QStringLiteral("true"));
  QCOMPARE(entry->field("tasted"), QStringLiteral("true"));
  QVERIFY(!entry->field("description").isEmpty());
  QVERIFY(!entry->field("comments").isEmpty());
  QVERIFY(!entry->field("label").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setOptions(exporter.options() | Tellico::Export::ExportImages);
  exporter.setEntries(coll->entries());

  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());
  QCOMPARE(coll2->title(), coll->title());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      QCOMPARE(f->name() + e2->field(f), f->name() + e1->field(f));
    }
  }
}

void GCstarTest::testCoin() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-coin.gcs"));
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Coin);
  QCOMPARE(coll->entryCount(), 1);
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("denomination"), QStringLiteral("0.05"));
  QCOMPARE(entry->field("year"), QStringLiteral("1974"));
  QCOMPARE(entry->field("currency"), QStringLiteral("USD"));
  QCOMPARE(entry->field("diameter"), QStringLiteral("12.7"));
  QCOMPARE(entry->field("estimate"), QStringLiteral("5"));
  QCOMPARE(entry->field("grade"), QStringLiteral("Mint State-65"));
  QCOMPARE(entry->field("country"), QStringLiteral("australia"));
  QCOMPARE(entry->field("location"), QStringLiteral("current"));
  QCOMPARE(entry->field("service"), QStringLiteral("PCGS"));
  QCOMPARE(TABLES(entry, "metal").count(), 2);
  QCOMPARE(TABLES(entry, "metal").at(1), QStringLiteral("metal2"));
  QVERIFY(!entry->field("comments").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());
  QCOMPARE(coll2->title(), coll->title());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // skip images
      if(f->type() != Tellico::Data::Field::Image) {
        QCOMPARE(f->name() + e2->field(f), f->name() + e1->field(f));
      }
    }
  }
}

void GCstarTest::testCustomFields() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-book.gcs"));
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 2);
  // should be translated somehow
  QCOMPARE(coll->title(), QStringLiteral("GCstar Import"));
  QVERIFY(importer.canImport(coll->type()));

  // test custom fields
  Tellico::Data::FieldPtr field = coll->fieldByName(QStringLiteral("gcsfield1"));
  QVERIFY(field);
  QCOMPARE(field->name(), QStringLiteral("gcsfield1"));
  QCOMPARE(field->title(), QStringLiteral("New boolean"));
  QCOMPARE(field->category(), QStringLiteral("User fields"));
  QCOMPARE(field->type(), Tellico::Data::Field::Bool);

  field = coll->fieldByName(QStringLiteral("gcsfield2"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("New choice"));
  QCOMPARE(field->type(), Tellico::Data::Field::Choice);
  QCOMPARE(field->allowed(), QStringList() << QStringLiteral("yes")
                                           << QStringLiteral("no")
                                           << QStringLiteral("maybe"));

  field = coll->fieldByName(QStringLiteral("gcsfield3"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("New rating"));
  QCOMPARE(field->type(), Tellico::Data::Field::Rating);
  QCOMPARE(field->property(QStringLiteral("minimum")), QStringLiteral("1"));
  QCOMPARE(field->property(QStringLiteral("maximum")), QStringLiteral("5"));

  field = coll->fieldByName(QStringLiteral("gcsfield4"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("New field"));
  QCOMPARE(field->type(), Tellico::Data::Field::Line);

  field = coll->fieldByName(QStringLiteral("gcsfield5"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("New image"));
  QCOMPARE(field->type(), Tellico::Data::Field::Image);

  field = coll->fieldByName(QStringLiteral("gcsfield6"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("New long field"));
  QCOMPARE(field->type(), Tellico::Data::Field::Para);

  field = coll->fieldByName(QStringLiteral("gcsfield7"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("New date"));
  QCOMPARE(field->type(), Tellico::Data::Field::Date);

  field = coll->fieldByName(QStringLiteral("gcsfield8"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("New number"));
  QCOMPARE(field->type(), Tellico::Data::Field::Number);
  QCOMPARE(field->defaultValue(), QStringLiteral("2"));

  field = coll->fieldByName(QStringLiteral("gcsfield9"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("dependency"));
  QCOMPARE(field->type(), Tellico::Data::Field::Line);
  QCOMPARE(field->property(QStringLiteral("template")), QStringLiteral("%{gcsfield1},%{gcsfield2}"));

  field = coll->fieldByName(QStringLiteral("gcsfield10"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("list"));
  QCOMPARE(field->type(), Tellico::Data::Field::Table);
  QCOMPARE(field->property(QStringLiteral("columns")), QStringLiteral("1"));

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("gcsfield1"), QStringLiteral("true"));
  QCOMPARE(entry->field("gcsfield2"), QStringLiteral("maybe"));
  QCOMPARE(entry->field("gcsfield3"), QStringLiteral("3"));
  QCOMPARE(entry->field("gcsfield4"), QStringLiteral("random value"));
  QCOMPARE(entry->field("gcsfield6"), QStringLiteral("all\nthe best \nstuff"));
  QCOMPARE(entry->field("gcsfield7"), QStringLiteral("2013-03-31"));
  QCOMPARE(entry->field("gcsfield9"), QStringLiteral("true,maybe"));
  QCOMPARE(TABLES(entry, "gcsfield10").count(), 2);
  QCOMPARE(TABLES(entry, "gcsfield10").at(1), QStringLiteral("list2"));

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());

  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();
  QVERIFY(coll2);

  foreach(Tellico::Data::FieldPtr f1, coll->fields()) {
    Tellico::Data::FieldPtr f2 = coll2->fieldByName(f1->name());
    QVERIFY2(f2, qPrintable(f1->name()));
    QCOMPARE(f1->name(), f2->name());
    QCOMPARE(f1->title(), f2->title());
    QCOMPARE(f1->category(), f2->category());
    QCOMPARE(f1->allowed(), f2->allowed());
    QCOMPARE(f1->type(), f2->type());
    QCOMPARE(f1->flags(), f2->flags());
    QCOMPARE(f1->formatType(), f2->formatType());
    QCOMPARE(f1->description(), f2->description());
    QCOMPARE(f1->defaultValue(), f2->defaultValue());
    QCOMPARE(f1->property(QStringLiteral("minimum")), f2->property(QStringLiteral("minimum")));
    QCOMPARE(f1->property(QStringLiteral("maximum")), f2->property(QStringLiteral("maximum")));
    QCOMPARE(f1->property(QStringLiteral("columns")), f2->property(QStringLiteral("columns")));
    QCOMPARE(f1->property(QStringLiteral("template")), f2->property(QStringLiteral("template")));
  }

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    QCOMPARE(TABLES(e2, "gcsfield10").count(), 2);
    QCOMPARE(TABLES(e2, "gcsfield10").at(1), QStringLiteral("list2"));
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // skip images
      if(f->type() != Tellico::Data::Field::Image) {
        QCOMPARE(f->name() + e1->field(f), f->name() + e2->field(f));
      }
    }
  }
}

void GCstarTest::testCustomCollection() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-custom.gcs"));
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Base);
  QCOMPARE(coll->entryCount(), 2);
  QCOMPARE(coll->title(), QStringLiteral("GCstar Import"));

  // test custom fields
  Tellico::Data::FieldPtr field = coll->fieldByName(QStringLiteral("gcsfield1"));
  QVERIFY(field);
  QCOMPARE(field->name(), QStringLiteral("gcsfield1"));
  QCOMPARE(field->title(), QStringLiteral("Picture Right"));
  QCOMPARE(field->type(), Tellico::Data::Field::Image);

  field = coll->fieldByName(QStringLiteral("gcsfield16"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("Long Description"));
  QCOMPARE(field->type(), Tellico::Data::Field::Para);

  field = coll->fieldByName(QStringLiteral("gcsfield25"));
  QVERIFY(field);
  QCOMPARE(field->title(), QStringLiteral("Location"));
  QCOMPARE(field->type(), Tellico::Data::Field::Choice);
  QCOMPARE(field->flags(), Tellico::Data::Field::AllowGrouped);
  QCOMPARE(field->allowed(), QStringList() << QStringLiteral("EDC")
                                           << QStringLiteral("Deposito")
                                           << QStringLiteral("Estante"));
  QCOMPARE(field->defaultValue(), QStringLiteral("Deposito"));

  field = coll->fieldByName(QStringLiteral("gcsfield14"));
  QVERIFY(field);
  QCOMPARE(field->type(), Tellico::Data::Field::Date);

  field = coll->fieldByName(QStringLiteral("gcsfield28"));
  QVERIFY(field);
  QCOMPARE(field->type(), Tellico::Data::Field::Number);
  QCOMPARE(field->property(QStringLiteral("minimum")), QStringLiteral("17"));
  QCOMPARE(field->property(QStringLiteral("maximum")), QStringLiteral("30"));

  Tellico::Data::EntryPtr entry = coll->entryById(71);
  QVERIFY(entry);
  QCOMPARE(entry->field("gcsfield11"), QStringLiteral("Dark Hollow"));
  QCOMPARE(entry->field("gcsfield14"), QStringLiteral("2025-06-01"));
  QCOMPARE(entry->field("gcsfield16"), QStringLiteral("DARK HOLLOW CUTLERY D2 STEEL WOOD HANDLE FIXED BLADE KNIFE KNIFE"));
  // image field
  QCOMPARE(entry->field("gcsfield1"), QStringLiteral("dde5bf2cbd90fad8635a26dfb362e0ff.png"));
}
