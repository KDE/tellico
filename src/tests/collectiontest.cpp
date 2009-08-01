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
#include "collectiontest.h"
#include "collectiontest.moc"

#include "../collection.h"

QTEST_KDEMAIN_CORE( CollectionTest )

void CollectionTest::testEmpty() {
  Tellico::Data::CollPtr nullColl;
  Tellico::Data::Collection coll(QLatin1String("Title"));

  QVERIFY(nullColl.isNull());
  QCOMPARE(coll.entryCount(), 0);
  QCOMPARE(coll.type(), Tellico::Data::Collection::Base);
  QVERIFY(coll.fields().isEmpty());
  QCOMPARE(coll.title(), QLatin1String("Title"));
}

void CollectionTest::testCollection() {
  Tellico::Data::Collection coll(true); // add default field

  QCOMPARE(coll.entryCount(), 0);
  QCOMPARE(coll.type(), Tellico::Data::Collection::Base);
  QCOMPARE(coll.fields().count(), 2);
  QVERIFY(coll.hasField(QLatin1String("title")));
  QVERIFY(coll.hasField(QLatin1String("id")));
  QVERIFY(coll.peopleFields().isEmpty());
  QVERIFY(coll.imageFields().isEmpty());
  QVERIFY(!coll.hasImages());
}
