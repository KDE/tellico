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
#include "alexandriaimporttest.h"
#include "alexandriaimporttest.moc"

#include "../translators/alexandriaimporter.h"
#include "../collections/bookcollection.h"
#include "../images/imagefactory.h"

QTEST_KDEMAIN_CORE( AlexandriaImportTest )

#define QL1(x) QString::fromLatin1(x)

void AlexandriaImportTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void AlexandriaImportTest::testImport() {
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
}
