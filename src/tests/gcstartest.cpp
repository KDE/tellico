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

#include "qtest_kde.h"
#include "gcstartest.h"
#include "gcstartest.moc"

#include "../translators/gcstarimporter.h"
#include "../translators/gcstarexporter.h"
#include "../collections/collectioninitializer.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"

#include <kstandarddirs.h>

#define FIELDS(entry, fieldName) Tellico::FieldFormat::splitValue(entry->field(fieldName))

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
  QCOMPARE(coll->entryCount(), 1);
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

void GCstarTest::testVideo() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test-video.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 2);

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
