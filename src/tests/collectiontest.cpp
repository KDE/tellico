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

#include "collectiontest.h"

#include "../collection.h"
#include "../field.h"
#include "../entry.h"
#include "../collectionfactory.h"
#include "../collections/collectioninitializer.h"
#include "../translators/tellicoxmlexporter.h"
#include "../images/imagefactory.h"
#include "../document.h"

#include <KProcess>

#include <QTest>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( CollectionTest )

class TestResolver : public Tellico::MergeConflictResolver {
public:
  TestResolver(Tellico::MergeConflictResolver::Result ret) : m_ret(ret) {};
  Tellico::MergeConflictResolver::Result resolve(Tellico::Data::EntryPtr,
                                                 Tellico::Data::EntryPtr,
                                                 Tellico::Data::FieldPtr,
                                                 const QString& value1 = QString(),
                                                 const QString& value2 = QString()) {
    Q_UNUSED(value1);
    Q_UNUSED(value2);
    return m_ret;
  }

private:
  Tellico::MergeConflictResolver::Result m_ret;
};

void CollectionTest::initTestCase() {
  Tellico::ImageFactory::init();
  // need to register the collection types
  Tellico::CollectionInitializer ci;
}

void CollectionTest::testEmpty() {
  Tellico::Data::CollPtr nullColl;
  QVERIFY(!nullColl);

  Tellico::Data::Collection coll(false, QLatin1String("Title"));

  QCOMPARE(coll.id(), 1);
  QCOMPARE(coll.entryCount(), 0);
  QCOMPARE(coll.type(), Tellico::Data::Collection::Base);
  QVERIFY(coll.fields().isEmpty());
  QCOMPARE(coll.title(), QLatin1String("Title"));
}

void CollectionTest::testCollection() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default fields

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
  QCOMPARE(entry1->field(QLatin1String("id")), QLatin1String("1"));
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
  QCOMPARE(entry2->field(QLatin1String("id")), QLatin1String("2"));
  // check created and modified values
  QCOMPARE(entry2->field(QLatin1String("cdate")), weekAgo.toString(Qt::ISODate));
  QCOMPARE(entry2->field(QLatin1String("mdate")), yesterday.toString(Qt::ISODate));

  // check that mdate gets updates
  entry2->setField(QLatin1String("title"), QLatin1String("new title"));
  QCOMPARE(entry2->field(QLatin1String("cdate")), weekAgo.toString(Qt::ISODate));
  QCOMPARE(entry2->field(QLatin1String("mdate")), QDate::currentDate().toString(Qt::ISODate));

  // check Bug 361622 - properly handling empty rows in table
  Tellico::Data::FieldPtr tableField(new Tellico::Data::Field(QLatin1String("table"), QLatin1String("Table"), Tellico::Data::Field::Table));
  tableField->setFormatType(Tellico::FieldFormat::FormatName);
  coll->addField(tableField);
  QString tableValue = QLatin1String("Value1")
                     + Tellico::FieldFormat::rowDelimiterString()
                     + Tellico::FieldFormat::rowDelimiterString()
                     + QLatin1String("Value2");
  entry2->setField(QLatin1String("table"), tableValue);
  QCOMPARE(entry2->formattedField(QLatin1String("table")), tableValue);
  QCOMPARE(QSet<QString>::fromList(entry2->groupNamesByFieldName(QLatin1String("table"))),
           QSet<QString>() << QLatin1String("Value1") << QLatin1String("Value2"));
}

void CollectionTest::testFields() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default fields

  QCOMPARE(coll->fields().count(), 4);
  QVERIFY(coll->peopleFields().isEmpty());
  QVERIFY(coll->imageFields().isEmpty());

  Tellico::Data::FieldPtr aField(new Tellico::Data::Field(QLatin1String("author"),
                                                          QLatin1String("Author")));
  aField->setFlags(Tellico::Data::Field::AllowMultiple | Tellico::Data::Field::AllowGrouped);
  aField->setFormatType(Tellico::FieldFormat::FormatName);
  QCOMPARE(coll->addField(aField), true);
  QVERIFY(coll->hasField(QLatin1String("author")));
  QCOMPARE(coll->defaultGroupField(), QLatin1String("author"));

  QCOMPARE(coll->fields().count(), 5);
  QCOMPARE(coll->peopleFields().count(), 1);
  QVERIFY(coll->imageFields().isEmpty());
  QVERIFY(!coll->hasImages());
  QCOMPARE(coll->fieldsByCategory(QLatin1String("General")).size(), 2);

  Tellico::Data::FieldPtr bField(new Tellico::Data::Field(QLatin1String("cover"),
                                                          QLatin1String("Cover"),
                                                          Tellico::Data::Field::Image));
  QCOMPARE(coll->addField(bField), true);
  QVERIFY(coll->hasField(QLatin1String("cover")));
  QVERIFY(!coll->hasField(QLatin1String("Ccover")));

  QCOMPARE(coll->fields().count(), 6);
  QCOMPARE(coll->peopleFields().count(), 1);
  QCOMPARE(coll->imageFields().count(), 1);
  QVERIFY(coll->hasImages());

  QStringList cats = coll->fieldCategories();
  QCOMPARE(cats.size(), 3);
  QVERIFY(cats.contains(QLatin1String("General")));
  QVERIFY(cats.contains(QLatin1String("Personal")));
  QVERIFY(cats.contains(QLatin1String("Cover")));

  const QStringList names = coll->fieldNames();
  QCOMPARE(names.size(), 6);
  QVERIFY(names.contains(QLatin1String("author")));
  QVERIFY(names.contains(QLatin1String("cover")));

  const QStringList titles = coll->fieldTitles();
  QCOMPARE(titles.size(), 6);
  QVERIFY(titles.contains(QLatin1String("Author")));
  QVERIFY(titles.contains(QLatin1String("Cover")));

  QCOMPARE(coll->fieldByName(QLatin1String("author")), aField);
  QCOMPARE(coll->fieldByTitle(QLatin1String("Author")), aField);
  QCOMPARE(coll->fieldNameByTitle(QLatin1String("Author")), QLatin1String("author"));
  QCOMPARE(coll->fieldNameByTitle(QLatin1String("author")), QString());
  QCOMPARE(coll->fieldTitleByName(QLatin1String("Author")), QString());
  QCOMPARE(coll->fieldTitleByName(QLatin1String("author")), QLatin1String("Author"));

  QVERIFY(coll->removeField(QLatin1String("cover")));
  QVERIFY(!coll->hasField(QLatin1String("cover")));
  QCOMPARE(coll->fields().count(), 5);
  QVERIFY(!coll->hasImages());
  QCOMPARE(coll->fieldTitleByName(QLatin1String("cover")), QString());
  QCOMPARE(coll->fieldCategories().size(), 2);

  Tellico::Data::FieldPtr cField(new Tellico::Data::Field(QLatin1String("editor"),
                                                          QLatin1String("Editor")));
  cField->setFlags(Tellico::Data::Field::AllowGrouped);
  cField->setFormatType(Tellico::FieldFormat::FormatName);
  cField->setCategory(QLatin1String("People"));

  // since the field name does not match an existing field, modifying should fail
  QVERIFY(!coll->modifyField(cField));
  cField->setName(QLatin1String("author"));
  QVERIFY(coll->modifyField(cField));
  QCOMPARE(coll->fieldByName(QLatin1String("author")), cField);
  QCOMPARE(coll->fieldByTitle(QLatin1String("Author")), Tellico::Data::FieldPtr());
  QCOMPARE(coll->fieldByTitle(QLatin1String("Editor")), cField);
  QCOMPARE(coll->peopleFields().count(), 1);

  cats = coll->fieldCategories();
  QCOMPARE(cats.size(), 3);
  QVERIFY(cats.contains(QLatin1String("General")));
  QVERIFY(cats.contains(QLatin1String("Personal")));
  QVERIFY(cats.contains(QLatin1String("People")));

  QCOMPARE(coll->fieldsByCategory(QLatin1String("General")).size(), 1);
  QCOMPARE(coll->fieldsByCategory(QLatin1String("People")).size(), 1);

  coll->clear();
  QVERIFY(coll->fields().isEmpty());
  QVERIFY(coll->peopleFields().isEmpty());
  QVERIFY(coll->imageFields().isEmpty());
  QVERIFY(coll->fieldCategories().isEmpty());
  QVERIFY(coll->defaultGroupField().isEmpty());
  QCOMPARE(coll->fieldByName(QLatin1String("author")), Tellico::Data::FieldPtr());
  QCOMPARE(coll->fieldByTitle(QLatin1String("Editor")), Tellico::Data::FieldPtr());
}

void CollectionTest::testDerived() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default field

  Tellico::Data::FieldPtr aField(new Tellico::Data::Field(QLatin1String("author"),
                                                          QLatin1String("Author")));
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
  const QString xmllint = QStandardPaths::findExecutable(QLatin1String("xmllint"));
  if(xmllint.isEmpty()) {
    QSKIP("This test requires xmllint", SkipAll);
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
      case Tellico::Data::Field::Number: entry1->setField(field, QLatin1String("1")); break;
      case Tellico::Data::Field::Rating: entry1->setField(field, QLatin1String("1")); break;
      case Tellico::Data::Field::Date:   entry1->setField(field, QLatin1String("2009-01-10")); break;
      case Tellico::Data::Field::Bool:   entry1->setField(field, QLatin1String("true")); break;
      case Tellico::Data::Field::Choice: entry1->setField(field, field->allowed().first()); break;
      default: break;
    }
  }

  Tellico::Export::TellicoXMLExporter exporter(coll);
  exporter.setEntries(coll->entries());

  KProcess proc;
  proc.setProgram(QLatin1String("xmllint"),
                  QStringList() << QLatin1String("--noout")
                                << QLatin1String("--dtdvalid")
                                << QFINDTESTDATA("../../tellico.dtd")
                                << QLatin1String("-"));

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
  entry1->setField(QLatin1String("title"), QLatin1String("title1"));
  coll->addEntries(entry1);
  QCOMPARE(coll->entryCount(), 1);

  // this is how Controller::slotCopySelectedEntries() does it
  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(*entry1));
  coll->addEntries(entry2);
  QCOMPARE(coll->entryCount(), 2);

  QCOMPARE(entry1->title(), entry2->title());
  QVERIFY(entry1->id() != entry2->id());

  bool ret = Tellico::Data::Document::mergeEntry(entry1, entry2);
  QCOMPARE(ret, true);

  TestResolver cancelMerge(Tellico::MergeConflictResolver::CancelMerge);
  ret = Tellico::Data::Document::mergeEntry(entry1, entry2, &cancelMerge);
  QCOMPARE(ret, true);

  entry2->setField(QLatin1String("title"), QLatin1String("title2"));

  ret = Tellico::Data::Document::mergeEntry(entry1, entry2, &cancelMerge);
  QCOMPARE(ret, false);
  QCOMPARE(entry1->title(), QLatin1String("title1"));
  QCOMPARE(entry2->title(), QLatin1String("title2"));

  TestResolver keepFirst(Tellico::MergeConflictResolver::KeepFirst);
  ret = Tellico::Data::Document::mergeEntry(entry1, entry2, &keepFirst);
  QCOMPARE(ret, true);
  QCOMPARE(entry1->title(), QLatin1String("title1"));
  // the second entry never gets changed
  QCOMPARE(entry2->title(), QLatin1String("title2"));

  entry2->setField(QLatin1String("title"), QLatin1String("title2"));

  TestResolver keepSecond(Tellico::MergeConflictResolver::KeepSecond);
  ret = Tellico::Data::Document::mergeEntry(entry1, entry2, &keepSecond);
  QCOMPARE(ret, true);
  QCOMPARE(entry1->title(), QLatin1String("title2"));
  QCOMPARE(entry2->title(), QLatin1String("title2"));

  entry1->setField(QLatin1String("title"), QLatin1String("title1"));

  // returns true, ("merge successful") even if values were not merged
  ret = Tellico::Data::Document::mergeEntry(entry1, entry2);
  QCOMPARE(ret, true);
  QCOMPARE(entry1->title(), QLatin1String("title1"));
  QCOMPARE(entry2->title(), QLatin1String("title2"));
}

void CollectionTest::testMergeFields() {
  // here, we want to verify that when entries and fields from outside a collection are merged in
  // the allowed values for the Choice fields are retained in the same order, and new values are only
  // added if they are used
  Tellico::Data::CollPtr coll1 = Tellico::CollectionFactory::collection(Tellico::Data::Collection::Game, true);
  Tellico::Data::CollPtr coll2 = Tellico::CollectionFactory::collection(Tellico::Data::Collection::Game, true);

  // modify the allowed values for  "platform" in collection 1
  Tellico::Data::FieldPtr platform1 = coll1->fieldByName(QLatin1String("platform"));
  QVERIFY(platform1);
  QStringList newValues1 = QStringList() << QLatin1String("PSP") << QLatin1String("Xbox 360");
  platform1->setAllowed(newValues1);
  QVERIFY(coll1->modifyField(platform1));
  QCOMPARE(platform1->allowed(), newValues1);

  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll2));
  entry2->setField(QLatin1String("platform"), QLatin1String("PlayStation"));
  QCOMPARE(entry2->field(QLatin1String("platform")), QLatin1String("PlayStation"));
  coll2->addEntries(entry2);

  QPair<Tellico::Data::FieldList, Tellico::Data::FieldList> p = Tellico::Data::Document::mergeFields(coll1,
                                       Tellico::Data::FieldList() << coll2->fieldByName(QLatin1String("platform")),
                                       Tellico::Data::EntryList() << entry2);

  Tellico::Data::FieldList modifiedFields = p.first;
  QCOMPARE(modifiedFields.count(), 1);
  // this is the zinger right here. The list of allowed values should be the original
  // with only the new existing value tacked on the end
  QCOMPARE(modifiedFields.first()->allowed(), newValues1 << entry2->field(QLatin1String("platform")));

  Tellico::Data::FieldList addedFields = p.second;
  QVERIFY(addedFields.isEmpty());
}

void CollectionTest::testAppendCollection() {
  // appending a collection adds new fields, merges existing one, and add new entries
  // the new entries should belong to the original collection and the existing entries should 
  // remain in the source collection
  Tellico::Data::CollPtr coll1 = Tellico::CollectionFactory::collection(Tellico::Data::Collection::Game, true);
  Tellico::Data::CollPtr coll2 = Tellico::CollectionFactory::collection(Tellico::Data::Collection::Game, true);

  // modify the allowed values for  "platform" in collection 1
  Tellico::Data::FieldPtr platform1 = coll1->fieldByName(QLatin1String("platform"));
  QVERIFY(platform1);
  QStringList newValues1 = QStringList() << QLatin1String("My Box");
  platform1->setAllowed(newValues1);
  QVERIFY(coll1->modifyField(platform1));
  // add a new field
  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(QLatin1String("test"), QLatin1String("test")));
  QVERIFY(coll1->addField(field1));

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll1));
  QCOMPARE(entry1->collection(), coll1);
  coll1->addEntries(entry1);

  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll2));
  QCOMPARE(entry2->collection(), coll2);
  coll2->addEntries(entry2);

  // append coll1 into coll2
  Tellico::Data::Document::appendCollection(coll2, coll1);
  // verify that the test field was added
  QVERIFY(coll2->hasField(QLatin1String("test")));
  // verified that the modified field was merged
  Tellico::Data::FieldPtr platform2 = coll2->fieldByName(QLatin1String("platform"));
  QVERIFY(platform2);
  QVERIFY(platform2->allowed().contains(QLatin1String("My Box")));

  // coll2 should have two entries now, both with proper parent
  QCOMPARE(coll2->entryCount(), 2);
  Tellico::Data::EntryList e2 = coll2->entries();
  QCOMPARE(e2.at(0)->collection(), coll2);
  QCOMPARE(e2.at(1)->collection(), coll2);

  QCOMPARE(coll1->entryCount(), 1);
  Tellico::Data::EntryList e1 = coll1->entries();
  QCOMPARE(e1.at(0)->collection(), coll1);
}
