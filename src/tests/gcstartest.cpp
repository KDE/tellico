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
#include "gcstartest.moc"
#include "qtest_kde.h"

#include "../translators/gcstarimporter.h"
#include "../translators/gcstarexporter.h"
#include "../collections/collectioninitializer.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"

#include <kstandarddirs.h>

#define FIELDS(entry, fieldName) Tellico::FieldFormat::splitValue(entry->field(fieldName))
#define TABLES(entry, fieldName) Tellico::FieldFormat::splitTable(entry->field(fieldName))

QTEST_KDEMAIN_CORE( GCstarTest )

void GCstarTest::initTestCase() {
  Tellico::ImageFactory::init();
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  // need to register the collection types
  Tellico::CollectionInitializer ci;
}

void GCstarTest::testBook() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test-book.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 2);
  // should be translated somehow
  QCOMPARE(coll->title(), QLatin1String("GCstar Import"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("The Reason for God"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("2008"));
  QCOMPARE(FIELDS(entry, "author").count(), 2);
  QCOMPARE(FIELDS(entry, "author").first(), QLatin1String("Timothy Keller"));
  QCOMPARE(entry->field("isbn"), QLatin1String("978-0-525-95049-3"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Dutton Adult"));
  QCOMPARE(FIELDS(entry, "genre").count(), 2);
  QCOMPARE(FIELDS(entry, "genre").at(0), QLatin1String("non-fiction"));
  QCOMPARE(FIELDS(entry, "keyword").count(), 2);
  QCOMPARE(FIELDS(entry, "keyword").at(0), QLatin1String("tag1"));
  QCOMPARE(FIELDS(entry, "keyword").at(1), QLatin1String("tag2"));
  // file has rating of 4, Tellico uses half the rating of GCstar, so it should be 2
  QCOMPARE(entry->field("rating"), QLatin1String("2"));
  QCOMPARE(FIELDS(entry, "language").count(), 1);
  QCOMPARE(FIELDS(entry, "language").at(0), QLatin1String("English"));

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());

  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(!coll2.isNull());
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
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test-comicbook.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::ComicBook);
  QCOMPARE(coll->entryCount(), 1);
  // should be translated somehow
  QCOMPARE(coll->title(), QLatin1String("GCstar Import"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("title"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("2010"));
  QCOMPARE(entry->field("series"), QLatin1String("series"));
  QCOMPARE(entry->field("issue"), QLatin1String("1"));
  QCOMPARE(FIELDS(entry, "writer").count(), 2);
  QCOMPARE(FIELDS(entry, "writer").first(), QLatin1String("writer1"));
  QCOMPARE(entry->field("isbn"), QLatin1String("1234567890"));
  QCOMPARE(entry->field("artist"), QLatin1String("illustrator"));
  QCOMPARE(entry->field("publisher"), QLatin1String("publisher"));
  QCOMPARE(entry->field("colorist"), QLatin1String("colourist"));
  QCOMPARE(entry->field("category"), QLatin1String("category"));
  QCOMPARE(entry->field("format"), QLatin1String("format"));
  QCOMPARE(entry->field("collection"), QLatin1String("collection"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("29/08/2010"));
  QCOMPARE(entry->field("pur_price"), QLatin1String("12.99"));
  QCOMPARE(entry->field("numberboards"), QLatin1String("1"));
  QCOMPARE(entry->field("signed"), QLatin1String("true"));
  // file has rating of 4, Tellico uses half the rating of GCstar, so it should be 2
  QCOMPARE(entry->field("rating"), QLatin1String("2"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("comments").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());

  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(!coll2.isNull());
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
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test-video.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 3);

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("The Man from Snowy River"));
  QCOMPARE(entry->field("year"), QLatin1String("1982"));
  QCOMPARE(FIELDS(entry, "director").count(), 1);
  QCOMPARE(FIELDS(entry, "director").first(), QLatin1String("George Miller"));
  QCOMPARE(FIELDS(entry, "nationality").count(), 1);
  QCOMPARE(FIELDS(entry, "nationality").first(), QLatin1String("Australia"));
  QCOMPARE(entry->field("medium"), QLatin1String("DVD"));
  QCOMPARE(entry->field("running-time"), QLatin1String("102"));
  QCOMPARE(FIELDS(entry, "genre").count(), 4);
  QCOMPARE(FIELDS(entry, "genre").at(0), QLatin1String("Drama"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QCOMPARE(castList.count(), 10);
  QCOMPARE(castList.at(0), QLatin1String("Tom Burlinson::Jim Craig"));
  QCOMPARE(castList.at(2), QLatin1String("Kirk Douglas::Harrison / Spur"));
  QCOMPARE(FIELDS(entry, "keyword").count(), 2);
  QCOMPARE(FIELDS(entry, "keyword").at(0), QLatin1String("tag2"));
  QCOMPARE(FIELDS(entry, "keyword").at(1), QLatin1String("tag1"));
  QCOMPARE(entry->field("rating"), QLatin1String("3"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("comments").isEmpty());

  entry = coll->entryById(4);
  QVERIFY(!entry.isNull());
  castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QCOMPARE(castList.count(), 11);
  QCOMPARE(castList.at(0), QLatin1String("Famke Janssen::Marnie Watson"));
  QCOMPARE(entry->field("location"), QLatin1String("On Hard Drive"));

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(!coll2.isNull());
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
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test-music.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("Lifesong"));
  QCOMPARE(entry->field("year"), QLatin1String("2005"));
  QCOMPARE(FIELDS(entry, "artist").count(), 1);
  QCOMPARE(FIELDS(entry, "artist").first(), QLatin1String("Casting Crowns"));
  QCOMPARE(FIELDS(entry, "label").count(), 1);
  QCOMPARE(FIELDS(entry, "label").first(), QLatin1String("Beach Street Records"));
  QCOMPARE(entry->field("medium"), QLatin1String("Compact Disc"));
  QCOMPARE(FIELDS(entry, "genre").count(), 2);
  QCOMPARE(FIELDS(entry, "genre").at(0), QLatin1String("Electronic"));
  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field("track"));
  QCOMPARE(trackList.count(), 11);
  QCOMPARE(trackList.at(1), QLatin1String("Praise You In This Storm::Casting Crowns::4:59"));
  QCOMPARE(FIELDS(entry, "producer").count(), 1);
  QCOMPARE(FIELDS(entry, "producer").at(0), QLatin1String("Mark A. Miller"));
  QCOMPARE(FIELDS(entry, "composer").count(), 4);
  QCOMPARE(FIELDS(entry, "composer").at(1), QLatin1String("David Hunt"));
  QCOMPARE(entry->field("cdate"), QLatin1String("2009-09-22"));

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(!coll2.isNull());
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
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test-videogame.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Game);
  QCOMPARE(coll->entryCount(), 2);

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("Halo 3"));
  QCOMPARE(entry->field("year"), QLatin1String("2007"));
  QCOMPARE(entry->field("platform"), QLatin1String("Xbox 360"));
  QCOMPARE(FIELDS(entry, "developer").count(), 1);
  QCOMPARE(FIELDS(entry, "developer").first(), QLatin1String("Bungie Studios"));
  QCOMPARE(FIELDS(entry, "publisher").count(), 1);
  QCOMPARE(FIELDS(entry, "publisher").first(), QLatin1String("Microsoft Games Studios"));
  QCOMPARE(FIELDS(entry, "genre").count(), 3);
  QCOMPARE(FIELDS(entry, "genre").at(0), QLatin1String("Action"));
  QCOMPARE(entry->field("cdate"), QLatin1String("2009-09-24"));
  QVERIFY(!entry->field("description").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(!coll2.isNull());
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
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test-boardgame.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::BoardGame);
  QCOMPARE(coll->entryCount(), 2);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("Risk"));
  QCOMPARE(entry->field("year"), QLatin1String("1959"));
  QCOMPARE(FIELDS(entry, "designer").count(), 2);
  QCOMPARE(FIELDS(entry, "designer").at(1), QLatin1String("Michael I. Levin"));
  QCOMPARE(FIELDS(entry, "publisher").count(), 11);
  QCOMPARE(FIELDS(entry, "publisher").at(1), QLatin1String("Borras Plana S.A."));
  QCOMPARE(FIELDS(entry, "mechanism").count(), 3);
  QCOMPARE(FIELDS(entry, "mechanism").at(1), QLatin1String("Dice Rolling"));
  QCOMPARE(FIELDS(entry, "genre").count(), 1);
  QCOMPARE(FIELDS(entry, "genre").at(0), QLatin1String("Wargame"));
  QVERIFY(!entry->field("description").isEmpty());
  QVERIFY(!entry->field("comments").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(!coll2.isNull());
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
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test-wine.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Wine);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("vintage"), QLatin1String("1990"));
  QCOMPARE(entry->field("producer"), QLatin1String("producer"));
  QCOMPARE(entry->field("type"), QLatin1String("Red Wine"));
  QCOMPARE(entry->field("country"), QLatin1String("australia"));
  QCOMPARE(entry->field("quantity"), QLatin1String("1"));
  QCOMPARE(FIELDS(entry, "varietal").count(), 2);
  QCOMPARE(FIELDS(entry, "varietal").at(1), QLatin1String("grape2"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("28/08/2010"));
  QCOMPARE(entry->field("pur_price"), QLatin1String("12.99"));
  QCOMPARE(entry->field("appellation"), QLatin1String("designation"));
  QCOMPARE(entry->field("distinction"), QLatin1String("distinction"));
  QCOMPARE(entry->field("soil"), QLatin1String("soil"));
  QCOMPARE(entry->field("alcohol"), QLatin1String("12"));
  QCOMPARE(entry->field("volume"), QLatin1String("750"));
  QCOMPARE(entry->field("rating"), QLatin1String("3"));
  QCOMPARE(entry->field("gift"), QLatin1String("true"));
  QCOMPARE(entry->field("tasted"), QLatin1String("true"));
  QVERIFY(!entry->field("description").isEmpty());
  QVERIFY(!entry->field("comments").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(!coll2.isNull());
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

void GCstarTest::testCoin() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test-coin.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Coin);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("denomination"), QLatin1String("0.05"));
  QCOMPARE(entry->field("year"), QLatin1String("1974"));
  QCOMPARE(entry->field("currency"), QLatin1String("USD"));
  QCOMPARE(entry->field("diameter"), QLatin1String("12.7"));
  QCOMPARE(entry->field("estimate"), QLatin1String("5"));
  QCOMPARE(entry->field("grade"), QLatin1String("Mint State-65"));
  QCOMPARE(entry->field("country"), QLatin1String("australia"));
  QCOMPARE(entry->field("location"), QLatin1String("current"));
  QCOMPARE(entry->field("service"), QLatin1String("PCGS"));
  QCOMPARE(TABLES(entry, "metal").count(), 2);
  QCOMPARE(TABLES(entry, "metal").at(1), QLatin1String("metal2"));
  QVERIFY(!entry->field("comments").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(!coll2.isNull());
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
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test-book.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 2);
  // should be translated somehow
  QCOMPARE(coll->title(), QLatin1String("GCstar Import"));

  // test custom fields
  Tellico::Data::FieldPtr field = coll->fieldByName(QLatin1String("gcsfield1"));
  QVERIFY(!field.isNull());
  QCOMPARE(field->name(), QLatin1String("gcsfield1"));
  QCOMPARE(field->title(), QLatin1String("New boolean"));
  QCOMPARE(field->category(), QLatin1String("User fields"));
  QCOMPARE(field->type(), Tellico::Data::Field::Bool);

  field = coll->fieldByName(QLatin1String("gcsfield2"));
  QVERIFY(!field.isNull());
  QCOMPARE(field->title(), QLatin1String("New choice"));
  QCOMPARE(field->type(), Tellico::Data::Field::Choice);
  QCOMPARE(field->allowed(), QStringList() << QLatin1String("yes")
                                           << QLatin1String("no")
                                           << QLatin1String("maybe"));

  field = coll->fieldByName(QLatin1String("gcsfield3"));
  QVERIFY(!field.isNull());
  QCOMPARE(field->title(), QLatin1String("New rating"));
  QCOMPARE(field->type(), Tellico::Data::Field::Rating);
  QCOMPARE(field->property(QLatin1String("minimum")), QLatin1String("1"));
  QCOMPARE(field->property(QLatin1String("maximum")), QLatin1String("5"));

  field = coll->fieldByName(QLatin1String("gcsfield4"));
  QVERIFY(!field.isNull());
  QCOMPARE(field->title(), QLatin1String("New field"));
  QCOMPARE(field->type(), Tellico::Data::Field::Line);

  field = coll->fieldByName(QLatin1String("gcsfield5"));
  QVERIFY(!field.isNull());
  QCOMPARE(field->title(), QLatin1String("New image"));
  QCOMPARE(field->type(), Tellico::Data::Field::Image);

  field = coll->fieldByName(QLatin1String("gcsfield6"));
  QVERIFY(!field.isNull());
  QCOMPARE(field->title(), QLatin1String("New long field"));
  QCOMPARE(field->type(), Tellico::Data::Field::Para);

  field = coll->fieldByName(QLatin1String("gcsfield7"));
  QVERIFY(!field.isNull());
  QCOMPARE(field->title(), QLatin1String("New date"));
  QCOMPARE(field->type(), Tellico::Data::Field::Date);

  field = coll->fieldByName(QLatin1String("gcsfield8"));
  QVERIFY(!field.isNull());
  QCOMPARE(field->title(), QLatin1String("New number"));
  QCOMPARE(field->type(), Tellico::Data::Field::Number);
  QCOMPARE(field->defaultValue(), QLatin1String("2"));

  field = coll->fieldByName(QLatin1String("gcsfield9"));
  QVERIFY(!field.isNull());
  QCOMPARE(field->title(), QLatin1String("dependency"));
  QCOMPARE(field->type(), Tellico::Data::Field::Line);
  QCOMPARE(field->property(QLatin1String("template")), QLatin1String("%{gcsfield1},%{gcsfield2}"));

  field = coll->fieldByName(QLatin1String("gcsfield10"));
  QVERIFY(!field.isNull());
  QCOMPARE(field->title(), QLatin1String("list"));
  QCOMPARE(field->type(), Tellico::Data::Field::Table);
  QCOMPARE(field->property(QLatin1String("columns")), QLatin1String("1"));

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("gcsfield1"), QLatin1String("true"));
  QCOMPARE(entry->field("gcsfield2"), QLatin1String("maybe"));
  QCOMPARE(entry->field("gcsfield3"), QLatin1String("3"));
  QCOMPARE(entry->field("gcsfield4"), QLatin1String("random value"));
  QCOMPARE(entry->field("gcsfield6"), QLatin1String("all\nthe best \nstuff"));
  QCOMPARE(entry->field("gcsfield7"), QLatin1String("2013-03-31"));
  QCOMPARE(entry->field("gcsfield9"), QLatin1String("true,maybe"));
  QCOMPARE(TABLES(entry, "gcsfield10").count(), 2);
  QCOMPARE(TABLES(entry, "gcsfield10").at(1), QLatin1String("list2"));

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());

  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();
  QVERIFY(!coll2.isNull());

  foreach(Tellico::Data::FieldPtr f1, coll->fields()) {
    Tellico::Data::FieldPtr f2 = coll2->fieldByName(f1->name());
    QVERIFY2(f2, f1->name().toLatin1());
    QCOMPARE(f1->name(), f2->name());
    QCOMPARE(f1->title(), f2->title());
    QCOMPARE(f1->category(), f2->category());
    QCOMPARE(f1->allowed(), f2->allowed());
    QCOMPARE(f1->type(), f2->type());
    QCOMPARE(f1->flags(), f2->flags());
    QCOMPARE(f1->formatType(), f2->formatType());
    QCOMPARE(f1->description(), f2->description());
    QCOMPARE(f1->defaultValue(), f2->defaultValue());
    QCOMPARE(f1->property(QLatin1String("minimum")), f2->property(QLatin1String("minimum")));
    QCOMPARE(f1->property(QLatin1String("maximum")), f2->property(QLatin1String("maximum")));
    QCOMPARE(f1->property(QLatin1String("columns")), f2->property(QLatin1String("columns")));
    QCOMPARE(f1->property(QLatin1String("template")), f2->property(QLatin1String("template")));
  }

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    QCOMPARE(TABLES(e2, "gcsfield10").count(), 2);
    QCOMPARE(TABLES(e2, "gcsfield10").at(1), QLatin1String("list2"));
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // skip images
      if(f->type() != Tellico::Data::Field::Image) {
        QCOMPARE(f->name() + e1->field(f), f->name() + e2->field(f));
      }
    }
  }
}
