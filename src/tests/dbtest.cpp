/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "dbtest.h"
#include "qtest_kde.h"

#include "../datastore/sqlitedb.h"
#include "../datastore/dbimporter.h"
#include "../datastore/dbfieldreader.h"
#include "../collections/bookcollection.h"

QTEST_KDEMAIN_CORE( DBTest )

void DBTest::initTestCase() {
}

void DBTest::testTables() {
  Tellico::SqliteDB db;
  QVERIFY(db.createTables());
}

void DBTest::testImport() {
  Tellico::SqliteDB db;
  QVERIFY(db.createTables());

  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true));
  QVERIFY(coll);

  Tellico::DBImporter importer(&db);
  QVERIFY(importer.import(coll));

  Tellico::DBFieldReader reader(&db);
  QList<Tellico::Data::FieldRef> fields = reader.allFields();
  QCOMPARE(coll->fields().count(), fields.count());
  foreach(const Tellico::Data::FieldRef& ref, fields) {
    Tellico::Data::FieldPtr ptr = coll->fieldByName(ref.name());
    QVERIFY(ptr);
    QCOMPARE(ref.name(), ptr->name());
  }
}
