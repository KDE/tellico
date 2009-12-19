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
#include "../field.h"
#include "../entry.h"

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

  // also verify that the empty string is included in list of group names
  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(QLatin1String("test"), QLatin1String("test")));
  coll->addField(field1);
  QStringList groupNames = entry1->groupNamesByFieldName(QLatin1String("test"));
  QCOMPARE(groupNames.count(), 1);
  QVERIFY(groupNames.at(0).isEmpty());

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

void CollectionTest::testDerived() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default field

  Tellico::Data::FieldPtr aField(new Tellico::Data::Field(QLatin1String("author"), QLatin1String("Author")));
  aField->setFlags(Tellico::Data::Field::AllowMultiple);
  aField->setFormatType(Tellico::FieldFormat::FormatName);
  coll->addField(aField);

  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField(QLatin1String("author"), QLatin1String("Albert Einstein; Niels Bohr"));
  coll->addEntries(entry);

  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QLatin1String("test"), QLatin1String("Test")));
  field->setProperty(QLatin1String("template"), QLatin1String("%{author}"));
  field->setFlags(Tellico::Data::Field::Derived);
  field->setFormatType(Tellico::FieldFormat::FormatName);
  coll->addField(field);

  QCOMPARE(entry->field(QLatin1String("test")), QLatin1String("Albert Einstein; Niels Bohr"));

  field->setProperty(QLatin1String("template"), QLatin1String("%{test3}"));

  Tellico::Data::FieldPtr field2(new Tellico::Data::Field(QLatin1String("test2"), QLatin1String("Test")));
  field2->setProperty(QLatin1String("template"), QLatin1String("%{test}"));
  field2->setFlags(Tellico::Data::Field::Derived);
  coll->addField(field2);

  Tellico::Data::FieldPtr field3(new Tellico::Data::Field(QLatin1String("test3"), QLatin1String("Test")));
  field3->setProperty(QLatin1String("template"), QLatin1String("%{test3:1}"));
  field3->setFlags(Tellico::Data::Field::Derived);
  coll->addField(field3);

  // recursive, so template should be empty now
  QCOMPARE(field3->property(QLatin1String("template")), QString());

  // now test all the possible format options
  field->setProperty(QLatin1String("template"), QLatin1String("%{author:1}"));
  QCOMPARE(entry->field(QLatin1String("test")), QLatin1String("Albert Einstein"));

  field->setProperty(QLatin1String("template"), QLatin1String("%{author:1/l}"));
  QCOMPARE(entry->field(QLatin1String("test")), QLatin1String("albert einstein"));

  field->setProperty(QLatin1String("template"), QLatin1String("%{author:1/u}"));
  QCOMPARE(entry->field(QLatin1String("test")), QLatin1String("ALBERT EINSTEIN"));

  field->setProperty(QLatin1String("template"), QLatin1String("%{author:1}"));
  QCOMPARE(entry->formattedField(QLatin1String("test"), Tellico::FieldFormat::ForceFormat), QLatin1String("Einstein, Albert"));

  field->setProperty(QLatin1String("template"), QLatin1String("%{author:2}"));
  QCOMPARE(entry->field(QLatin1String("test")), QLatin1String("Niels Bohr"));

  field->setProperty(QLatin1String("template"), QLatin1String("%{author:-1}"));
  QCOMPARE(entry->field(QLatin1String("test")), QLatin1String("Niels Bohr"));

  field->setProperty(QLatin1String("template"), QLatin1String("%{author:-1}"));
  QCOMPARE(entry->formattedField(QLatin1String("test"), Tellico::FieldFormat::ForceFormat), QLatin1String("Bohr, Niels"));

  field->setProperty(QLatin1String("template"), QLatin1String("%{author:-2}"));
  QCOMPARE(entry->field(QLatin1String("test")), QLatin1String("Albert Einstein"));
}

void CollectionTest::testValue() {
  QFETCH(QString, string);
  QFETCH(QString, formatted);
  QFETCH(int, typeInt);

  Tellico::FieldFormat::Type type = static_cast<Tellico::FieldFormat::Type>(typeInt);

  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default field

  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(QLatin1String("test"), QLatin1String("Test")));
  field1->setFlags(Tellico::Data::Field::AllowMultiple);
  field1->setFormatType(type);
  coll->addField(field1);

  Tellico::Data::FieldPtr field2(new Tellico::Data::Field(QLatin1String("table"), QLatin1String("Table"), Tellico::Data::Field::Table));
  field2->setFormatType(type);
  coll->addField(field2);

  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);

  entry->setField(field1, string);

  const QString dummy = Tellico::FieldFormat::columnDelimiterString() + QLatin1String("dummy, the; Dummy, The; the dummy");
  entry->setField(field2, string + dummy);

  QCOMPARE(entry->formattedField(field1, Tellico::FieldFormat::ForceFormat), formatted);
  QCOMPARE(entry->formattedField(field2, Tellico::FieldFormat::ForceFormat), formatted + dummy);
}

void CollectionTest::testValue_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("formatted");
  QTest::addColumn<int>("typeInt");

  QTest::newRow("test1") << "name" << "Name" << int(Tellico::FieldFormat::FormatName);
  QTest::newRow("test2") << "name1; name2" << "Name1; Name2" << int(Tellico::FieldFormat::FormatName);
  QTest::newRow("test3") << "Bob Dylan;Randy Quaid" << "Dylan, Bob; Quaid, Randy" << int(Tellico::FieldFormat::FormatName);
  QTest::newRow("test4") << "the return of the king" << "Return of the King, The" << int(Tellico::FieldFormat::FormatTitle);
  QTest::newRow("test5") << "the return of the king;the who" << "Return of the King, The; Who, The" << int(Tellico::FieldFormat::FormatTitle);
}

