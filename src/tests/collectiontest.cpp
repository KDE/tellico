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
  QVERIFY(nullColl.isNull());

  Tellico::Data::Collection coll(false, QLatin1String("Title"));

  QCOMPARE(coll.entryCount(), 0);
  QCOMPARE(coll.type(), Tellico::Data::Collection::Base);
  QVERIFY(coll.fields().isEmpty());
  QCOMPARE(coll.title(), QLatin1String("Title"));
}

void CollectionTest::testCollection() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default field

  QCOMPARE(coll->entryCount(), 0);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Base);
  QCOMPARE(coll->fields().count(), 4);
  QVERIFY(coll->hasField(QLatin1String("title")));
  QVERIFY(coll->hasField(QLatin1String("id")));
  QVERIFY(coll->hasField(QLatin1String("cdate")));
  QVERIFY(coll->hasField(QLatin1String("mdate")));
  QVERIFY(coll->peopleFields().isEmpty());
  QVERIFY(coll->imageFields().isEmpty());
  QVERIFY(!coll->hasImages());

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  coll->addEntries(entry1);

  // check derived value
  QCOMPARE(entry1->field(QLatin1String("id")), QLatin1String("0"));
  // check created and modified values
  QCOMPARE(entry1->field(QLatin1String("cdate")), QDate::currentDate().toString(Qt::ISODate));
  QCOMPARE(entry1->field(QLatin1String("mdate")), QDate::currentDate().toString(Qt::ISODate));

  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  // add created and modified dates from earlier, to make sure they don't get overwritten
  QDate weekAgo = QDate::currentDate().addDays(-7);
  QDate yesterday = QDate::currentDate().addDays(-1);
  entry2->setField(QLatin1String("cdate"), weekAgo.toString(Qt::ISODate));
  entry2->setField(QLatin1String("mdate"), yesterday.toString(Qt::ISODate));
  coll->addEntries(entry2);

  // check derived value
  QCOMPARE(entry2->field(QLatin1String("id")), QLatin1String("1"));
  // check created and modified values
  QCOMPARE(entry2->field(QLatin1String("cdate")), weekAgo.toString(Qt::ISODate));
  QCOMPARE(entry2->field(QLatin1String("mdate")), yesterday.toString(Qt::ISODate));
}
