/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include <kstandarddirs.h>

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
  QCOMPARE(entry->fields("author", false).count(), 2);
  QCOMPARE(entry->fields("author", false).first(), QLatin1String("Timothy Keller"));
  QCOMPARE(entry->field("isbn"), QLatin1String("978-0-525-95049-3"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Dutton Adult"));
  QCOMPARE(entry->fields("genre", false).count(), 2);
  QCOMPARE(entry->fields("genre", false).at(0), QLatin1String("non-fiction"));
  QCOMPARE(entry->fields("keyword", false).count(), 2);
  QCOMPARE(entry->fields("keyword", false).at(0), QLatin1String("tag1"));
  QCOMPARE(entry->fields("keyword", false).at(1), QLatin1String("tag2"));
  // file has rating of 4, Tellico uses half the rating of GCstar, so it should be 2
  QCOMPARE(entry->field("rating"), QLatin1String("2"));
  QCOMPARE(entry->fields("language", false).count(), 1);
  QCOMPARE(entry->fields("language", false).at(0), QLatin1String("English"));

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
  QCOMPARE(entry->fields("director", false).count(), 1);
  QCOMPARE(entry->fields("director", false).first(), QLatin1String("George Miller"));
  QCOMPARE(entry->fields("nationality", false).count(), 1);
  QCOMPARE(entry->fields("nationality", false).first(), QLatin1String("Australia"));
  QCOMPARE(entry->field("medium"), QLatin1String("DVD"));
  QCOMPARE(entry->field("running-time"), QLatin1String("102"));
  QCOMPARE(entry->fields("genre", false).count(), 4);
  QCOMPARE(entry->fields("genre", false).at(0), QLatin1String("Drama"));
  QCOMPARE(entry->fields("cast", false).count(), 10);
  QCOMPARE(entry->fields("cast", false).at(0), QLatin1String("Tom Burlinson::Jim Craig"));
  QCOMPARE(entry->fields("cast", false).at(2), QLatin1String("Kirk Douglas::Harrison / Spur"));
  QCOMPARE(entry->fields("keyword", false).count(), 2);
  QCOMPARE(entry->fields("keyword", false).at(0), QLatin1String("tag2"));
  QCOMPARE(entry->fields("keyword", false).at(1), QLatin1String("tag1"));
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
  QCOMPARE(entry->fields("designer", false).count(), 2);
  QCOMPARE(entry->fields("designer", false).at(1), QLatin1String("Michael I. Levin"));
  QCOMPARE(entry->fields("publisher", false).count(), 11);
  QCOMPARE(entry->fields("publisher", false).at(1), QLatin1String("Borras Plana S.A."));
  QCOMPARE(entry->fields("mechanism", false).count(), 3);
  QCOMPARE(entry->fields("mechanism", false).at(1), QLatin1String("Dice Rolling"));
  QCOMPARE(entry->fields("genre", false).count(), 1);
  QCOMPARE(entry->fields("genre", false).at(0), QLatin1String("Wargame"));
  QVERIFY(!entry->field("description").isEmpty());
  QVERIFY(!entry->field("comments").isEmpty());

  Tellico::Export::GCstarExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::GCstarImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  // no support for board games yet
  QEXPECT_FAIL("", "Support for exporting board games is not implemented yet", Abort);
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
