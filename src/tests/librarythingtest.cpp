/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "librarythingtest.h"

#include "../translators/librarythingimporter.h"
#include "../fieldformat.h"

#include <KLocalizedString>

#include <QTest>

QTEST_APPLESS_MAIN( LibraryThingTest )

void LibraryThingTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
}

void LibraryThingTest::testImport() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/librarything.json"));
   Tellico::Import::LibraryThingImporter importer(url);
  // shut the importer up about current collection
//  Tellico::Data::CollPtr tmpColl(new Tellico::Data::BibtexCollection(true));
//  importer.setCurrentCollection(tmpColl);

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 3);
  QCOMPARE(coll->title(), QStringLiteral("Your library"));

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Beautiful Community: Unity, Diversity, and the Church at Its Best"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("2020"));
  QCOMPARE(entry->field("author"), QStringLiteral("Irwyn L. Ince Jr."));
  QCOMPARE(entry->field("isbn"), QStringLiteral("0-8308-4831-2"));
  QCOMPARE(entry->field("language"), QStringLiteral("English"));
  QCOMPARE(entry->field("binding"), QStringLiteral("Paperback"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("IVP"));
  QCOMPARE(entry->field("cdate"), QStringLiteral("2020-09-27"));

  entry = coll->entryById(3);
  QVERIFY(entry);
  QCOMPARE(entry->field("genre"), QStringLiteral("Religion & Spirituality"));
//  TODO: option to include other fields?
//  QCOMPARE(entry->field("lcc"), QStringLiteral("BX9184 .A5"));
  QCOMPARE(entry->field("pages"), QStringLiteral("349"));
}
