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

#include "alexandriatest.h"
#include "alexandriatest.moc"
#include "qtest_kde.h"

#include "../translators/alexandriaimporter.h"
#include "../translators/alexandriaexporter.h"
#include "../collections/bookcollection.h"
#include "../images/imagefactory.h"

#include <ktempdir.h>

QTEST_KDEMAIN_CORE( AlexandriaTest )

#define QL1(x) QString::fromLatin1(x)

void AlexandriaTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void AlexandriaTest::testImport() {
  Tellico::Import::AlexandriaImporter importer;
  importer.setLibraryPath(QString::fromLatin1(KDESRCDIR) + "/data/alexandria/");

  // shut the importer up about current collection
  Tellico::Data::CollPtr tmpColl(new Tellico::Data::BookCollection(true));
  importer.setCurrentCollection(tmpColl);

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 2);
  // should be translated somehow
  QCOMPARE(coll->title(), QL1("My Books"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QCOMPARE(entry->field("title"), QL1("The Hallowed Hunt"));
  QCOMPARE(entry->field("comments"), QL1("first line<br/>second line"));

  entry = coll->entryById(2);
  QCOMPARE(entry->field("title"), QL1("Life Together"));
  QCOMPARE(entry->field("author"), QL1("Dietrich Bonhoeffer; My Other Author"));
  // translated
  QCOMPARE(entry->field("binding"), QL1("Hardback"));
  QCOMPARE(entry->field("isbn"), QL1("0-06-060853-6"));
  QCOMPARE(entry->field("pub_year"), QL1("1993"));
  QCOMPARE(entry->field("publisher"), QL1("Harper Collins"));
  QCOMPARE(entry->field("rating"), QL1("3"));
  QCOMPARE(entry->field("read"), QL1("true"));
  QCOMPARE(entry->field("loaned"), QL1(""));
  QVERIFY(!entry->field("comments").isEmpty());

  KTempDir outputDir;

  Tellico::Export::AlexandriaExporter exporter(coll);
  exporter.setEntries(coll->entries());
  exporter.setURL(outputDir.name());
  QVERIFY(exporter.exec());

  importer.setLibraryPath(outputDir.name() + "/.alexandria/" + coll->title());
  Tellico::Data::CollPtr coll2 = importer.collection();

  QVERIFY(!coll2.isNull());
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->title(), coll->title());
  QCOMPARE(coll2->entryCount(), coll->entryCount());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    // assume IDs stay the same
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // skip images
      if(f->type() != Tellico::Data::Field::Image) {
        QCOMPARE(f->name() + e1->field(f), f->name() + e2->field(f));
      }
    }
  }
}
