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

#include "alexandriatest.h"

#include "../translators/alexandriaimporter.h"
#include "../translators/alexandriaexporter.h"
#include "../collections/bookcollection.h"
#include "../images/imagefactory.h"

#include <KLocalizedString>

#include <QTest>
#include <QTemporaryDir>
#include <QStandardPaths>

QTEST_MAIN( AlexandriaTest )

#define QSL(x) QStringLiteral(x)

void AlexandriaTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::ImageFactory::init();
}

void AlexandriaTest::testImport() {
  Tellico::Import::AlexandriaImporter importer;
  QVERIFY(importer.canImport(Tellico::Data::Collection::Book));
  QVERIFY(!importer.canImport(Tellico::Data::Collection::Album));
  importer.setLibraryPath(QFINDTESTDATA("/data/alexandria/"));

  // shut the importer up about current collection
  Tellico::Data::CollPtr tmpColl(new Tellico::Data::BookCollection(true));
  importer.setCurrentCollection(tmpColl);

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 2);
  // should be translated somehow
  QCOMPARE(coll->title(), QSL("My Books"));
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QCOMPARE(entry->field(QSL("title")), QSL("The Hallowed Hunt"));
  QCOMPARE(entry->field(QSL("comments")), QSL("first line<br/>second line"));

  entry = coll->entryById(2);
  QCOMPARE(entry->field(QSL("title")), QSL("Life Together"));
  QCOMPARE(entry->field(QSL("author")), QSL("Dietrich Bonhoeffer; My Other Author"));
  // translated
  QCOMPARE(entry->field(QSL("binding")), QSL("Hardback"));
  QCOMPARE(entry->field(QSL("isbn")), QSL("0-06-060853-6"));
  QCOMPARE(entry->field(QSL("pub_year")), QSL("1993"));
  QCOMPARE(entry->field(QSL("publisher")), QSL("Harper Collins"));
  QCOMPARE(entry->field(QSL("rating")), QSL("3"));
  QCOMPARE(entry->field(QSL("read")), QSL("true"));
  QCOMPARE(entry->field(QSL("loaned")), QString());
  QVERIFY(!entry->field(QSL("comments")).isEmpty());

  QTemporaryDir outputDir;

  Tellico::Export::AlexandriaExporter exporter(coll);
  exporter.setEntries(coll->entries());
  exporter.setURL(QUrl::fromLocalFile(outputDir.path()));
  QVERIFY(exporter.exec());

  importer.setLibraryPath(outputDir.path() + QSL("/.alexandria/") + coll->title());
  Tellico::Data::CollPtr coll2 = importer.collection();

  QVERIFY(coll2);
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

void AlexandriaTest::testEscapeText() {
  // text escaping puts slashes in for quotes and remove control characters
  QString input(QStringLiteral("\"test \uFD3F") + QChar(0x90));
  QCOMPARE(Tellico::Export::AlexandriaExporter::escapeText(input), QStringLiteral("\\\"test \uFD3F"));
}

void AlexandriaTest::testWidget() {
  Tellico::Import::AlexandriaImporter importer;
  importer.setLibraryPath(QFINDTESTDATA("/data/alexandria/"));
  QVERIFY(!importer.libraryPath().isEmpty());
  QScopedPointer<QWidget> widget(importer.widget(nullptr));
  QVERIFY(widget);
  QVERIFY(importer.libraryPath().isEmpty()); // cleared when the widget is created
  auto coll = importer.collection();
  QVERIFY(!coll); // no collection with an empty library path
  importer.setLibraryPath(QFINDTESTDATA("/data/alexandria/"));
  coll = importer.collection();
  QVERIFY(coll);
}
