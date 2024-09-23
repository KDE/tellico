/***************************************************************************
    Copyright (C) 2009-2016 Robby Stephenson <robby@periapsis.org>
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

#include "collectiontest.h"

#include "../collection.h"
#include "../field.h"
#include "../entry.h"
#include "../collectionfactory.h"
#include "../collections/collectioninitializer.h"
#include "../collections/bookcollection.h"
#include "../collections/gamecollection.h"
#include "../translators/tellicoxmlexporter.h"
#include "../translators/tellicoimporter.h"
#include "../images/imagefactory.h"
#include "../document.h"
#include "../utils/mergeconflictresolver.h"

#include <KLocalizedString>
#include <KProcess>

#include <QTest>
#include <QStandardPaths>
#include <QRandomGenerator>
#include <QLoggingCategory>

QTEST_GUILESS_MAIN( CollectionTest )

class TestResolver : public Tellico::Merge::ConflictResolver {
public:
  TestResolver(Tellico::Merge::ConflictResolver::Result ret) : m_ret(ret) {};
  Tellico::Merge::ConflictResolver::Result resolve(Tellico::Data::EntryPtr,
                                                 Tellico::Data::EntryPtr,
                                                 Tellico::Data::FieldPtr,
                                                 const QString& value1 = QString(),
                                                 const QString& value2 = QString()) Q_DECL_OVERRIDE {
    Q_UNUSED(value1);
    Q_UNUSED(value2);
    return m_ret;
  }

private:
  Tellico::Merge::ConflictResolver::Result m_ret;
};

void CollectionTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  QLoggingCategory::setFilterRules(QStringLiteral("tellico.debug = true\ntellico.info = false"));
  Tellico::ImageFactory::init();
  // need to register the collection types
  Tellico::CollectionInitializer ci;
}

void CollectionTest::cleanupTestCase() {
  Tellico::ImageFactory::clean(true);
}

void CollectionTest::testEmpty() {
  Tellico::Data::CollPtr nullColl;
  QVERIFY(!nullColl);

  Tellico::Data::Collection coll(false, QStringLiteral("Title"));

  QCOMPARE(coll.id(), 1);
  QCOMPARE(coll.entryCount(), 0);
  QCOMPARE(coll.type(), Tellico::Data::Collection::Base);
  QVERIFY(coll.fields().isEmpty());
  QCOMPARE(coll.title(), QStringLiteral("Title"));
}

void CollectionTest::testCollection() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default fields

  QCOMPARE(coll->entryCount(), 0);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Base);
  QCOMPARE(coll->fields().count(), 4);
  QVERIFY(coll->hasField(QStringLiteral("title")));
  QVERIFY(coll->hasField(QStringLiteral("id")));
  QVERIFY(coll->hasField(QStringLiteral("cdate")));
  QVERIFY(coll->hasField(QStringLiteral("mdate")));
  QVERIFY(coll->peopleFields().isEmpty());
  QVERIFY(coll->imageFields().isEmpty());
  QVERIFY(!coll->hasImages());

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  coll->addEntries(entry1);

  // check derived value
  QCOMPARE(entry1->field(QStringLiteral("id")), QStringLiteral("1"));
  // check created and modified values
  QCOMPARE(entry1->field(QStringLiteral("cdate")), QDate::currentDate().toString(Qt::ISODate));
  QCOMPARE(entry1->field(QStringLiteral("mdate")), QDate::currentDate().toString(Qt::ISODate));

  // also verify that the empty string is included in list of group names
  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(QStringLiteral("test"), QStringLiteral("test")));
  coll->addField(field1);
  QStringList groupNames = entry1->groupNamesByFieldName(QStringLiteral("test"));
  QCOMPARE(groupNames.count(), 1);
  QVERIFY(groupNames.at(0).isEmpty());

  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  // add created and modified dates from earlier, to make sure they don't get overwritten
  QDate weekAgo = QDate::currentDate().addDays(-7);
  QDate yesterday = QDate::currentDate().addDays(-1);
  entry2->setField(QStringLiteral("cdate"), weekAgo.toString(Qt::ISODate));
  entry2->setField(QStringLiteral("mdate"), yesterday.toString(Qt::ISODate));
  coll->addEntries(entry2);

  // check derived value
  QCOMPARE(entry2->field(QStringLiteral("id")), QStringLiteral("2"));
  // check created and modified values
  QCOMPARE(entry2->field(QStringLiteral("cdate")), weekAgo.toString(Qt::ISODate));
  QCOMPARE(entry2->field(QStringLiteral("mdate")), yesterday.toString(Qt::ISODate));

  // check that mdate gets updates
  entry2->setField(QStringLiteral("title"), QStringLiteral("new title"));
  QCOMPARE(entry2->field(QStringLiteral("cdate")), weekAgo.toString(Qt::ISODate));
  QCOMPARE(entry2->field(QStringLiteral("mdate")), QDate::currentDate().toString(Qt::ISODate));

  // check Bug 361622 - properly handling empty rows in table
  Tellico::Data::FieldPtr tableField(new Tellico::Data::Field(QStringLiteral("table"), QStringLiteral("Table"), Tellico::Data::Field::Table));
  tableField->setFormatType(Tellico::FieldFormat::FormatName);
  coll->addField(tableField);
  QString tableValue = QStringLiteral("Value1")
                     + Tellico::FieldFormat::rowDelimiterString()
                     + Tellico::FieldFormat::rowDelimiterString()
                     + QStringLiteral("Value2");
  entry2->setField(QStringLiteral("table"), tableValue);
  QCOMPARE(entry2->formattedField(QStringLiteral("table")), tableValue);
  groupNames = entry2->groupNamesByFieldName(QStringLiteral("table"));
  QCOMPARE(groupNames.count(), 2);
  QVERIFY(groupNames.contains(QStringLiteral("Value1")));
  QVERIFY(groupNames.contains(QStringLiteral("Value2")));
}

void CollectionTest::testFields() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default fields

  QCOMPARE(coll->fields().count(), 4);
  QVERIFY(coll->peopleFields().isEmpty());
  QVERIFY(coll->imageFields().isEmpty());

  Tellico::Data::FieldPtr aField(new Tellico::Data::Field(QStringLiteral("author"),
                                                          QStringLiteral("Author")));
  aField->setFlags(Tellico::Data::Field::AllowMultiple | Tellico::Data::Field::AllowGrouped);
  aField->setFormatType(Tellico::FieldFormat::FormatName);
  QCOMPARE(coll->addField(aField), true);
  QVERIFY(coll->hasField(QStringLiteral("author")));
  QCOMPARE(coll->defaultGroupField(), QStringLiteral("author"));

  QCOMPARE(coll->fields().count(), 5);
  QCOMPARE(coll->peopleFields().count(), 1);
  QVERIFY(coll->imageFields().isEmpty());
  QVERIFY(!coll->hasImages());
  QCOMPARE(coll->fieldsByCategory(QStringLiteral("General")).size(), 2);

  Tellico::Data::FieldPtr bField(new Tellico::Data::Field(QStringLiteral("cover"),
                                                          QStringLiteral("Cover"),
                                                          Tellico::Data::Field::Image));
  QCOMPARE(coll->addField(bField), true);
  QVERIFY(coll->hasField(QStringLiteral("cover")));
  QVERIFY(!coll->hasField(QStringLiteral("Ccover")));

  QCOMPARE(coll->fields().count(), 6);
  QCOMPARE(coll->peopleFields().count(), 1);
  QCOMPARE(coll->imageFields().count(), 1);
  QVERIFY(coll->hasImages());

  QStringList cats = coll->fieldCategories();
  QCOMPARE(cats.size(), 3);
  QVERIFY(cats.contains(QStringLiteral("General")));
  QVERIFY(cats.contains(QStringLiteral("Personal")));
  QVERIFY(cats.contains(QStringLiteral("Cover")));

  const QStringList names = coll->fieldNames();
  QCOMPARE(names.size(), 6);
  QVERIFY(names.contains(QStringLiteral("author")));
  QVERIFY(names.contains(QStringLiteral("cover")));

  const QStringList titles = coll->fieldTitles();
  QCOMPARE(titles.size(), 6);
  QVERIFY(titles.contains(QStringLiteral("Author")));
  QVERIFY(titles.contains(QStringLiteral("Cover")));

  QCOMPARE(coll->fieldByName(QStringLiteral("author")), aField);
  QCOMPARE(coll->fieldByTitle(QStringLiteral("Author")), aField);
  QCOMPARE(coll->fieldNameByTitle(QStringLiteral("Author")), QStringLiteral("author"));
  QCOMPARE(coll->fieldNameByTitle(QStringLiteral("author")), QString());
  QCOMPARE(coll->fieldTitleByName(QStringLiteral("Author")), QString());
  QCOMPARE(coll->fieldTitleByName(QStringLiteral("author")), QStringLiteral("Author"));

  QVERIFY(coll->removeField(QStringLiteral("cover")));
  QVERIFY(!coll->hasField(QStringLiteral("cover")));
  QCOMPARE(coll->fields().count(), 5);
  QVERIFY(!coll->hasImages());
  QCOMPARE(coll->fieldTitleByName(QStringLiteral("cover")), QString());
  QCOMPARE(coll->fieldCategories().size(), 2);

  Tellico::Data::FieldPtr cField(new Tellico::Data::Field(QStringLiteral("editor"),
                                                          QStringLiteral("Editor")));
  cField->setFlags(Tellico::Data::Field::AllowGrouped);
  cField->setFormatType(Tellico::FieldFormat::FormatName);
  cField->setCategory(QStringLiteral("People"));

  // since the field name does not match an existing field, modifying should fail
  QVERIFY(!coll->modifyField(cField));
  cField->setName(QStringLiteral("author"));
  QVERIFY(coll->modifyField(cField));
  QCOMPARE(coll->fieldByName(QStringLiteral("author")), cField);
  QCOMPARE(coll->fieldByTitle(QStringLiteral("Author")), Tellico::Data::FieldPtr());
  QCOMPARE(coll->fieldByTitle(QStringLiteral("Editor")), cField);
  QCOMPARE(coll->peopleFields().count(), 1);

  cats = coll->fieldCategories();
  QCOMPARE(cats.size(), 3);
  QVERIFY(cats.contains(QStringLiteral("General")));
  QVERIFY(cats.contains(QStringLiteral("Personal")));
  QVERIFY(cats.contains(QStringLiteral("People")));

  QCOMPARE(coll->fieldsByCategory(QStringLiteral("General")).size(), 1);
  QCOMPARE(coll->fieldsByCategory(QStringLiteral("People")).size(), 1);

  coll->clear();
  QVERIFY(coll->fields().isEmpty());
  QVERIFY(coll->peopleFields().isEmpty());
  QVERIFY(coll->imageFields().isEmpty());
  QVERIFY(coll->fieldCategories().isEmpty());
  QVERIFY(coll->defaultGroupField().isEmpty());
  QCOMPARE(coll->fieldByName(QStringLiteral("author")), Tellico::Data::FieldPtr());
  QCOMPARE(coll->fieldByTitle(QStringLiteral("Editor")), Tellico::Data::FieldPtr());
}

void CollectionTest::testDerived() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default field

  Tellico::Data::FieldPtr aField(new Tellico::Data::Field(QStringLiteral("author"),
                                                          QStringLiteral("Author")));
  aField->setFlags(Tellico::Data::Field::AllowMultiple);
  aField->setFormatType(Tellico::FieldFormat::FormatName);
  coll->addField(aField);

  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField(QStringLiteral("author"), QStringLiteral("Albert Einstein; Niels Bohr"));
  coll->addEntries(entry);

  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("test"), QStringLiteral("Test")));
  field->setProperty(QStringLiteral("template"), QStringLiteral("%{author}"));
  field->setFlags(Tellico::Data::Field::Derived);
  field->setFormatType(Tellico::FieldFormat::FormatName);
  coll->addField(field);

  QCOMPARE(entry->field(QStringLiteral("test")), QStringLiteral("Albert Einstein; Niels Bohr"));

  field->setProperty(QStringLiteral("template"), QStringLiteral("%{test3}"));

  Tellico::Data::FieldPtr field2(new Tellico::Data::Field(QStringLiteral("test2"), QStringLiteral("Test")));
  field2->setProperty(QStringLiteral("template"), QStringLiteral("%{test}"));
  field2->setFlags(Tellico::Data::Field::Derived);
  coll->addField(field2);

  Tellico::Data::FieldPtr field3(new Tellico::Data::Field(QStringLiteral("test3"), QStringLiteral("Test")));
  field3->setProperty(QStringLiteral("template"), QStringLiteral("%{test3:1}"));
  field3->setFlags(Tellico::Data::Field::Derived);
  coll->addField(field3);

  // recursive, so template should be empty now
  QCOMPARE(field3->property(QStringLiteral("template")), QString());

  // now test all the possible format options
  field->setProperty(QStringLiteral("template"), QStringLiteral("%{author:1}"));
  QCOMPARE(entry->field(QStringLiteral("test")), QStringLiteral("Albert Einstein"));

  field->setProperty(QStringLiteral("template"), QStringLiteral("%{author:1/l}"));
  QCOMPARE(entry->field(QStringLiteral("test")), QStringLiteral("albert einstein"));

  field->setProperty(QStringLiteral("template"), QStringLiteral("%{author:1/u}"));
  QCOMPARE(entry->field(QStringLiteral("test")), QStringLiteral("ALBERT EINSTEIN"));

  field->setProperty(QStringLiteral("template"), QStringLiteral("%{author:1}"));
  QCOMPARE(entry->formattedField(QStringLiteral("test"), Tellico::FieldFormat::ForceFormat), QStringLiteral("Einstein, Albert"));

  field->setProperty(QStringLiteral("template"), QStringLiteral("%{author:2}"));
  QCOMPARE(entry->field(QStringLiteral("test")), QStringLiteral("Niels Bohr"));

  field->setProperty(QStringLiteral("template"), QStringLiteral("%{author:-1}"));
  QCOMPARE(entry->field(QStringLiteral("test")), QStringLiteral("Niels Bohr"));

  field->setProperty(QStringLiteral("template"), QStringLiteral("%{author:-1}"));
  QCOMPARE(entry->formattedField(QStringLiteral("test"), Tellico::FieldFormat::ForceFormat), QStringLiteral("Bohr, Niels"));

  field->setProperty(QStringLiteral("template"), QStringLiteral("%{author:-2}"));
  QCOMPARE(entry->field(QStringLiteral("test")), QStringLiteral("Albert Einstein"));
}

void CollectionTest::testValue() {
  QFETCH(QString, string);
  QFETCH(QString, formatted);
  QFETCH(int, typeInt);

  Tellico::FieldFormat::Type type = static_cast<Tellico::FieldFormat::Type>(typeInt);

  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default field

  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(QStringLiteral("test"), QStringLiteral("Test")));
  field1->setFlags(Tellico::Data::Field::AllowMultiple);
  field1->setFormatType(type);
  coll->addField(field1);

  Tellico::Data::FieldPtr field2(new Tellico::Data::Field(QStringLiteral("table"), QStringLiteral("Table"), Tellico::Data::Field::Table));
  field2->setFormatType(type);
  coll->addField(field2);

  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);

  entry->setField(field1, string);

  const QString dummy = Tellico::FieldFormat::columnDelimiterString() + QStringLiteral("dummy, the; Dummy, The; the dummy");
  entry->setField(field2, string + dummy);

  QCOMPARE(entry->formattedField(field1, Tellico::FieldFormat::ForceFormat), formatted);
  QCOMPARE(entry->formattedField(field2, Tellico::FieldFormat::ForceFormat), formatted.append(dummy));
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

void CollectionTest::testDtd() {
  const QString xmllint = QStandardPaths::findExecutable(QStringLiteral("xmllint"));
  if(xmllint.isEmpty()) {
    QSKIP("This test requires xmllint", SkipAll);
  }
  // xmllint doesn't seem to support spaces in path. Is this an XML thing?
  if(QFINDTESTDATA("../../tellico.dtd").contains(QRegularExpression(QStringLiteral("\\s")))) {
    QSKIP("This test prohibits whitespace in the build path", SkipAll);
  }

  QFETCH(int, typeInt);
  Tellico::Data::Collection::Type type = static_cast<Tellico::Data::Collection::Type>(typeInt);

  Tellico::Data::CollPtr coll = Tellico::CollectionFactory::collection(type, true);
  QVERIFY(coll);

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  coll->addEntries(entry1);

  foreach(Tellico::Data::FieldPtr field, coll->fields()) {
    switch(field->type()) {
      case Tellico::Data::Field::Line:   entry1->setField(field, field->title()); break;
      case Tellico::Data::Field::Para:   entry1->setField(field, field->title()); break;
      case Tellico::Data::Field::URL:    entry1->setField(field, field->title()); break;
      case Tellico::Data::Field::Table:  entry1->setField(field, field->title()); break;
      case Tellico::Data::Field::Image:  entry1->setField(field, field->title()); break;
      case Tellico::Data::Field::Number: entry1->setField(field, QStringLiteral("1")); break;
      case Tellico::Data::Field::Rating: entry1->setField(field, QStringLiteral("1")); break;
      case Tellico::Data::Field::Date:   entry1->setField(field, QStringLiteral("2009-01-10")); break;
      case Tellico::Data::Field::Bool:   entry1->setField(field, QStringLiteral("true")); break;
      case Tellico::Data::Field::Choice: entry1->setField(field, field->allowed().first()); break;
      default: break;
    }
  }

  Tellico::Export::TellicoXMLExporter exporter(coll);
  exporter.setEntries(coll->entries());

  KProcess proc;
  proc.setProgram(QStringLiteral("xmllint"),
                  QStringList() << QStringLiteral("--noout")
                                << QStringLiteral("--nonet")
                                << QStringLiteral("--nowarning")
                                << QStringLiteral("--dtdvalid")
                                << QFINDTESTDATA("../../tellico.dtd")
                                << QStringLiteral("-"));

  proc.start();
  proc.write(exporter.text().toUtf8());
  proc.closeWriteChannel();
  proc.waitForFinished();

  QCOMPARE(proc.exitCode(), 0);
}

void CollectionTest::testDtd_data() {
  QTest::addColumn<int>("typeInt");

  QTest::newRow("book")   << int(Tellico::Data::Collection::Book);
  QTest::newRow("video")  << int(Tellico::Data::Collection::Video);
  QTest::newRow("album")  << int(Tellico::Data::Collection::Album);
  QTest::newRow("bibtex") << int(Tellico::Data::Collection::Bibtex);
  QTest::newRow("comic")  << int(Tellico::Data::Collection::ComicBook);
  QTest::newRow("wine")   << int(Tellico::Data::Collection::Wine);
  QTest::newRow("coin")   << int(Tellico::Data::Collection::Coin);
  QTest::newRow("stamp")  << int(Tellico::Data::Collection::Stamp);
  QTest::newRow("card")   << int(Tellico::Data::Collection::Card);
  QTest::newRow("game")   << int(Tellico::Data::Collection::Game);
  QTest::newRow("file")   << int(Tellico::Data::Collection::File);
  QTest::newRow("board")  << int(Tellico::Data::Collection::BoardGame);
}

void CollectionTest::testDuplicate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true));

  QCOMPARE(coll->entryCount(), 0);

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QStringLiteral("title"), QStringLiteral("title1"));
  entry1->setField(QStringLiteral("cdate"), QStringLiteral("2019-01-01"));
  entry1->setField(QStringLiteral("mdate"), QStringLiteral("2019-04-01"));
  coll->addEntries(entry1);
  QCOMPARE(coll->entryCount(), 1);

  // this is how Controller::slotCopySelectedEntries() does it
  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(*entry1));
  QVERIFY(entry2->field(QStringLiteral("cdate")).isEmpty());
  QVERIFY(entry2->field(QStringLiteral("mdate")).isEmpty());
  coll->addEntries(entry2);
  QCOMPARE(coll->entryCount(), 2);

  QCOMPARE(entry1->title(), entry2->title());
  QVERIFY(entry1->id() != entry2->id());
  // creation date should reflect current date in the duplicated entry
  QVERIFY(entry1->field(QStringLiteral("cdate")) != entry2->field(QStringLiteral("cdate")));
  QCOMPARE(entry2->field(QStringLiteral("cdate")), QDate::currentDate().toString(Qt::ISODate));

  // also test operator= which is how ModifyEntries::swapValues() works
  Tellico::Data::Entry* entryPtr = new Tellico::Data::Entry(coll);
  *entryPtr = *entry1;
  Tellico::Data::EntryPtr entry3(entryPtr);
  QVERIFY(entry3->field(QStringLiteral("cdate")).isEmpty());
  QVERIFY(entry3->field(QStringLiteral("mdate")).isEmpty());
  coll->addEntries(entry3);
  QCOMPARE(coll->entryCount(), 3);

  QCOMPARE(entry1->title(), entry3->title());
  // entry id should be different
  QVERIFY(entry1->id() != entry3->id());
  // creation date should reflect current date in the duplicated entry
  QVERIFY(entry1->field(QStringLiteral("cdate")) != entry3->field(QStringLiteral("cdate")));
  QCOMPARE(entry3->field(QStringLiteral("cdate")), QDate::currentDate().toString(Qt::ISODate));

  bool ret = Tellico::Merge::mergeEntry(entry1, entry2);
  QCOMPARE(ret, true);

  TestResolver cancelMerge(Tellico::Merge::ConflictResolver::CancelMerge);
  ret = Tellico::Merge::mergeEntry(entry1, entry2, &cancelMerge);
  QCOMPARE(ret, true);

  entry2->setField(QStringLiteral("title"), QStringLiteral("title2"));

  ret = Tellico::Merge::mergeEntry(entry1, entry2, &cancelMerge);
  QCOMPARE(ret, false);
  QCOMPARE(entry1->title(), QStringLiteral("Title1"));
  QCOMPARE(entry2->title(), QStringLiteral("Title2"));

  TestResolver keepFirst(Tellico::Merge::ConflictResolver::KeepFirst);
  ret = Tellico::Merge::mergeEntry(entry1, entry2, &keepFirst);
  QCOMPARE(ret, true);
  QCOMPARE(entry1->title(), QStringLiteral("Title1"));
  // the second entry never gets changed
  QCOMPARE(entry2->title(), QStringLiteral("Title2"));

  entry2->setField(QStringLiteral("title"), QStringLiteral("title2"));

  TestResolver keepSecond(Tellico::Merge::ConflictResolver::KeepSecond);
  ret = Tellico::Merge::mergeEntry(entry1, entry2, &keepSecond);
  QCOMPARE(ret, true);
  QCOMPARE(entry1->title(), QStringLiteral("Title2"));
  QCOMPARE(entry2->title(), QStringLiteral("Title2"));

  entry1->setField(QStringLiteral("title"), QStringLiteral("title1"));

  // returns true, ("merge successful") even if values were not merged
  ret = Tellico::Merge::mergeEntry(entry1, entry2);
  QCOMPARE(ret, true);
  QCOMPARE(entry1->title(), QStringLiteral("Title1"));
  QCOMPARE(entry2->title(), QStringLiteral("Title2"));
}

void CollectionTest::testMergeFields() {
  // here, we want to verify that when entries and fields from outside a collection are merged in
  // the allowed values for the Choice fields are retained in the same order, and new values are only
  // added if they are used
  Tellico::Data::CollPtr coll1 = Tellico::CollectionFactory::collection(Tellico::Data::Collection::Game, true);
  Tellico::Data::CollPtr coll2 = Tellico::CollectionFactory::collection(Tellico::Data::Collection::Game, true);

  // modify the allowed values for  "platform" in collection 1
  Tellico::Data::FieldPtr platform1 = coll1->fieldByName(QStringLiteral("platform"));
  QVERIFY(platform1);
  QStringList newValues1 = QStringList() << QStringLiteral("PSP") << QStringLiteral("Xbox 360");
  platform1->setAllowed(newValues1);
  QVERIFY(coll1->modifyField(platform1));
  QCOMPARE(platform1->allowed(), newValues1);

  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll2));
  entry2->setField(QStringLiteral("platform"), QStringLiteral("PlayStation"));
  QCOMPARE(entry2->field(QStringLiteral("platform")), QStringLiteral("PlayStation"));
  coll2->addEntries(entry2);

  auto p = Tellico::Merge::mergeFields(coll1,
                                       Tellico::Data::FieldList() << coll2->fieldByName(QStringLiteral("platform")),
                                       Tellico::Data::EntryList() << entry2);

  Tellico::Data::FieldList modifiedFields = p.first;
  QCOMPARE(modifiedFields.count(), 1);
  // this is the zinger right here. The list of allowed values should be the original
  // with only the new existing value tacked on the end
  QCOMPARE(modifiedFields.first()->allowed(), newValues1 << entry2->field(QStringLiteral("platform")));

  Tellico::Data::FieldList addedFields = p.second;
  QVERIFY(addedFields.isEmpty());
}

void CollectionTest::testFieldsIntersection() {
  // simple test for the list intersection utility method
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true));
  Tellico::Data::FieldList imageFields = coll->imageFields();

  Tellico::Data::FieldList list = Tellico::listIntersection(imageFields, coll->fields());
  QCOMPARE(imageFields.count(), list.count());

  QBENCHMARK {
    // should be something less than 0.020 msecs :)
    Tellico::Data::FieldList list = Tellico::listIntersection(coll->fields(), coll->fields());
    Q_UNUSED(list);
  }
}

void CollectionTest::testAppendCollection() {
  // appending a collection adds new fields, merges existing one, and add new entries
  // the new entries should belong to the original collection and the existing entries should
  // remain in the source collection
  Tellico::Data::CollPtr coll1 = Tellico::CollectionFactory::collection(Tellico::Data::Collection::Game, true);
  Tellico::Data::CollPtr coll2 = Tellico::CollectionFactory::collection(Tellico::Data::Collection::Game, true);

  // modify the allowed values for  "platform" in collection 1
  Tellico::Data::FieldPtr platform1 = coll1->fieldByName(QStringLiteral("platform"));
  QVERIFY(platform1);
  QStringList newValues1 = QStringList() << QStringLiteral("My Box");
  platform1->setAllowed(newValues1);
  QVERIFY(coll1->modifyField(platform1));
  // add a new field
  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(QStringLiteral("test"), QStringLiteral("test")));
  QVERIFY(coll1->addField(field1));

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll1));
  QCOMPARE(entry1->collection(), coll1);
  coll1->addEntries(entry1);

  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll2));
  QCOMPARE(entry2->collection(), coll2);
  coll2->addEntries(entry2);

  // append coll1 into coll2
  bool structuralChange;
  Tellico::Data::Document::appendCollection(coll2, coll1, &structuralChange);
  QVERIFY(structuralChange);
  // verify that the test field was added
  QVERIFY(coll2->hasField(QStringLiteral("test")));
  // verified that the modified field was merged
  Tellico::Data::FieldPtr platform2 = coll2->fieldByName(QStringLiteral("platform"));
  QVERIFY(platform2);
  QVERIFY(platform2->allowed().contains(QStringLiteral("My Box")));

  // coll2 should have two entries now, both with proper parent
  QCOMPARE(coll2->entryCount(), 2);
  Tellico::Data::EntryList e2 = coll2->entries();
  QCOMPARE(e2.at(0)->collection(), coll2);
  QCOMPARE(e2.at(1)->collection(), coll2);

  QCOMPARE(coll1->entryCount(), 1);
  Tellico::Data::EntryList e1 = coll1->entries();
  QCOMPARE(e1.at(0)->collection(), coll1);
}

void CollectionTest::testMergeCollection() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/movies-many.tc"));

  Tellico::Import::TellicoImporter importer1(url);
  Tellico::Data::CollPtr coll1 = importer1.collection();
  QVERIFY(coll1);

  Tellico::Import::TellicoImporter importer2(url);
  Tellico::Data::CollPtr coll2 = importer2.collection();
  QVERIFY(coll2);

  Tellico::Data::EntryPtr entryToAdd(new Tellico::Data::Entry(coll2));
  QCOMPARE(entryToAdd->collection(), coll2);
  coll2->addEntries(entryToAdd);

  QCOMPARE(coll1->entryCount()+1, coll2->entryCount());

  // merge coll2 into coll1
  // first item is a vector of all entries that got added in the merge process
  // second item is a pair of entries that had their table field modified
  // typedef QVector< QPair<EntryPtr, QString> > PairVector;
  // typedef QPair<Data::EntryList, PairVector> MergePair;
  bool structuralChange;
  Tellico::Data::MergePair mergePair = Tellico::Data::Document::mergeCollection(coll1, coll2, &structuralChange);
  QCOMPARE(structuralChange, false);

  // one new entry was added
  QCOMPARE(mergePair.first.count(), 1);
  // no table fields edited either
  QVERIFY(mergePair.second.isEmpty());

  // check item count
  QCOMPARE(coll1->fields().count(), coll2->fields().count());
  QCOMPARE(coll1->entryCount(), coll2->entryCount());
}

void CollectionTest::testMergeBenchmark() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/movies-many.tc"));

  bool structuralChange;
  Tellico::Data::EntryList entriesToAdd;
  QBENCHMARK {
    Tellico::Import::TellicoImporter importer1(url, false /* load all images */);
    Tellico::Data::CollPtr coll1 = importer1.collection();
    QVERIFY(coll1);

    Tellico::Import::TellicoImporter importer2(url);
    Tellico::Data::CollPtr coll2 = importer2.collection();
    QVERIFY(coll2);

    entriesToAdd.clear();
    for(int i = 0; i < 500; ++i) {
      Tellico::Data::EntryPtr entryToAdd(new Tellico::Data::Entry(coll2));
      entryToAdd->setField(QStringLiteral("title"), QString::number(QRandomGenerator::global()->generate()));
      entryToAdd->setField(QStringLiteral("studio"), QString::number(i));
      entriesToAdd += entryToAdd;
    }
    coll2->addEntries(entriesToAdd);

    Tellico::Data::Document::mergeCollection(coll1, coll2, &structuralChange);
  }
}

void CollectionTest::testGamePlatform() {
  // test that the platform name guessing heuristic works on its own names
  for(int i = 1; i < Tellico::Data::GameCollection::LastPlatform; i++) {
    QString pName = Tellico::Data::GameCollection::platformName(Tellico::Data::GameCollection::GamePlatform(i));
    int pGuess = Tellico::Data::GameCollection::guessPlatform(pName);
    QCOMPARE(i, pGuess);
  }

  // test some specific platform names that some data sources might return
  // thegamesdb.net returns "Nintendo Game Boy"
  int gameBoy = Tellico::Data::GameCollection::guessPlatform(QStringLiteral("Nintendo Game Boy"));
  QCOMPARE(gameBoy, int(Tellico::Data::GameCollection::GameBoy));
  gameBoy = Tellico::Data::GameCollection::guessPlatform(QStringLiteral("gameboy"));
  QCOMPARE(gameBoy, int(Tellico::Data::GameCollection::GameBoy));
  gameBoy = Tellico::Data::GameCollection::guessPlatform(QStringLiteral("gameboy color"));
  QCOMPARE(gameBoy, int(Tellico::Data::GameCollection::GameBoyColor));
  gameBoy = Tellico::Data::GameCollection::guessPlatform(QStringLiteral("Gameboy Advance"));
  QCOMPARE(gameBoy, int(Tellico::Data::GameCollection::GameBoyAdvance));

  // don't match Nintendo Virtual Boy with Nintendo
  int guess = Tellico::Data::GameCollection::guessPlatform(QStringLiteral("Nintendo Virtual Boy"));
  QCOMPARE(guess, int(Tellico::Data::GameCollection::UnknownPlatform));
  guess = Tellico::Data::GameCollection::guessPlatform(QStringLiteral("Nintendo Entertainment System"));
  QCOMPARE(guess, int(Tellico::Data::GameCollection::Nintendo));

  QCOMPARE(Tellico::Data::GameCollection::normalizePlatform(QStringLiteral("Microsoft xboxone")),
           QStringLiteral("Xbox One"));
}

void CollectionTest::testEsrb() {
  // test that values for all rating enums are not empty and not repeated
  QSet<QString> values;
  for(int i = Tellico::Data::GameCollection::Unrated; i <= Tellico::Data::GameCollection::Pending; i++) {
    const QString esrb = Tellico::Data::GameCollection::esrbRating(Tellico::Data::GameCollection::EsrbRating(i));
    QVERIFY(!esrb.isEmpty());
    QVERIFY(!values.contains(esrb));
    values.insert(esrb);
  }
}

void CollectionTest::testNonTitle() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(false));
  QVERIFY(coll);
  QVERIFY(coll->fields().isEmpty());

  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(QStringLiteral("test1"), QStringLiteral("Test1")));
  coll->addField(field1);

  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField(QStringLiteral("test1"), QStringLiteral("non-title title"));
  coll->addEntries(entry);

  QCOMPARE(entry->title(), QStringLiteral("non-title title"));

  Tellico::Data::FieldPtr field2(new Tellico::Data::Field(QStringLiteral("test2"), QStringLiteral("Test")));
  field2->setFormatType(Tellico::FieldFormat::FormatTitle);
  coll->addField(field2);

  entry->setField(QStringLiteral("test2"), QStringLiteral("proxy title"));
  // since there's a new field formatted as a title, the entry title changes
  QCOMPARE(entry->title(), QStringLiteral("Proxy Title"));
}
