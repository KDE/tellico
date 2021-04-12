/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include "tellicomodeltest.h"
#include "modeltest.h"
#include "../models/entrymodel.h"
#include "../models/entryiconmodel.h"
#include "../models/entrysortmodel.h"
#include "../models/filtermodel.h"
#include "../models/entrygroupmodel.h"
#include "../models/groupsortmodel.h"
#include "../models/modeliterator.h"
#include "../models/entryselectionmodel.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../document.h"
#include "../entrygroup.h"
#include "../images/imagefactory.h"

#include <QTest>
#include <QSignalSpy>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( TellicoModelTest )

void TellicoModelTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
}

void TellicoModelTest::testEntryModel() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default fields
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QStringLiteral("title"), QStringLiteral("Star Wars"));
  coll->addEntries(entry1);

  Tellico::EntryModel entryModel(this);
  ModelTest test1(&entryModel);

  Tellico::EntryIconModel iconModel(this);
  ModelTest test2(&iconModel);

  Tellico::EntrySortModel sortModel(this);
  ModelTest test3(&sortModel);

  iconModel.setSourceModel(&entryModel);
  sortModel.setSourceModel(&entryModel);

  entryModel.setFields(coll->fields());
  entryModel.setEntries(coll->entries());
  QCOMPARE(entryModel.index(0, 0), entryModel.indexFromEntry(entry1));

  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(QStringLiteral("test"), QStringLiteral("test")));
  coll->addField(field1);
  entryModel.setFields(coll->fields());

  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  coll->addEntries(entry2);
  entryModel.addEntries(Tellico::Data::EntryList() << entry2);

  Tellico::Data::FieldPtr field2(new Tellico::Data::Field(QStringLiteral("test2"), QStringLiteral("test2")));
  coll->addField(field2);
  entryModel.addFields(Tellico::Data::FieldList() << field2);

  // Check FieldPtrRole in model data
  QModelIndex index = entryModel.index(0, entryModel.columnCount()-1);
  QVERIFY(index.isValid());
  QCOMPARE(entryModel.data(index, Tellico::FieldPtrRole),
           QVariant::fromValue(field2));
  //check FieldPtrRole in header model data
  QCOMPARE(entryModel.headerData(entryModel.columnCount()-1, Qt::Horizontal, Tellico::FieldPtrRole),
           QVariant::fromValue(field2));

  Tellico::FilterRule* rule1 = new Tellico::FilterRule(QStringLiteral("title"),
                                                       QStringLiteral("Star Wars"),
                                                       Tellico::FilterRule::FuncEquals);
  Tellico::FilterPtr filter(new Tellico::Filter(Tellico::Filter::MatchAny));
  filter->append(rule1);

  sortModel.setFilter(filter);

  entryModel.clear();
  entryModel.setFields(coll->fields());
  entryModel.setEntries(coll->entries());
  QCOMPARE(entryModel.index(0, 0), entryModel.indexFromEntry(entry1));

  Tellico::Data::FieldPtr field3(new Tellico::Data::Field(QStringLiteral("test"), QStringLiteral("test-new")));
  coll->modifyField(field3);
  QCOMPARE(coll->fields().count(), entryModel.columnCount(QModelIndex()));
  entryModel.modifyField(field2, field3);

  coll->removeField(field3);
  entryModel.removeFields(Tellico::Data::FieldList() << field3);
  QCOMPARE(coll->fields().count(), entryModel.columnCount(QModelIndex()));

  QCOMPARE(coll->entries().count(), entryModel.rowCount(QModelIndex()));
  coll->removeEntries(Tellico::Data::EntryList() << entry2);
  QCOMPARE(coll->entries().count() + 1, entryModel.rowCount(QModelIndex()));
  entryModel.removeEntries(Tellico::Data::EntryList() << entry2);
  QCOMPARE(coll->entries().count(), entryModel.rowCount(QModelIndex()));

  for(Tellico::ModelIterator eIt(&entryModel); eIt.entry(); ++eIt) {
    QVERIFY(eIt.isValid());
  }
}

void TellicoModelTest::testFilterModel() {
  Tellico::FilterModel filterModel(this);
  ModelTest test1(&filterModel);

  Tellico::FilterRule* rule1 = new Tellico::FilterRule(QStringLiteral("title"),
                                                       QStringLiteral("Star Wars"),
                                                       Tellico::FilterRule::FuncEquals);
  Tellico::FilterPtr filter(new Tellico::Filter(Tellico::Filter::MatchAny));
  filter->append(rule1);
  filterModel.addFilter(filter);

  Tellico::Data::CollPtr c = Tellico::Data::Document::self()->collection();
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(c));
  entry1->setField(QStringLiteral("title"), QStringLiteral("Star Wars"));
  c->addEntries(entry1);

  filterModel.clear();
  filterModel.addFilter(filter);
  QCOMPARE(filter, filterModel.filter(filterModel.index(0, 0)));
  QVERIFY(filterModel.indexContainsEntry(filterModel.index(0, 0), entry1));

  filterModel.invalidate(filterModel.index(0, 0));
  QCOMPARE(filter, filterModel.filter(filterModel.index(0, 0)));
  QVERIFY(filterModel.indexContainsEntry(filterModel.index(0, 0), entry1));
}

void TellicoModelTest::testGroupModel() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true)); // add default fields
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QStringLiteral("title"), QStringLiteral("Star Wars"));
  entry1->setField(QStringLiteral("author"), QStringLiteral("George Lucas"));
  coll->addEntries(entry1);

  Tellico::EntryGroupModel groupModel(this);
  ModelTest test1(&groupModel);

  Tellico::GroupSortModel sortModel(this);
  ModelTest test2(&sortModel);

  sortModel.setSourceModel(&groupModel);

  Tellico::Data::EntryGroupDict* dict = coll->entryGroupDictByName(QStringLiteral("author"));
  groupModel.addGroups(dict->values(), QString());
  QCOMPARE(sortModel.rowCount(), 1);

  for(Tellico::ModelIterator gIt(&groupModel); gIt.group(); ++gIt) {
    QVERIFY(gIt.isValid());
    Tellico::Data::EntryGroup* group = gIt.group();
    QVERIFY(group);
    QCOMPARE(group->groupName(), QStringLiteral("Lucas, George"));
    QCOMPARE(group->fieldName(), QStringLiteral("author"));
    QCOMPARE(group->size(), 1);
    QVERIFY(!group->hasEmptyGroupName());
  }
}

void TellicoModelTest::testSelectionModel() {
  qRegisterMetaType<Tellico::Data::EntryList>("Tellico::Data::EntryList");
  // this mimics the model dependencies used in mainwindow.cpp
  Tellico::EntryModel entryModel(this);
  ModelTest test1(&entryModel);
  Tellico::EntryIconModel iconModel(this);
  ModelTest test2(&iconModel);
  iconModel.setSourceModel(&entryModel);

  QItemSelectionModel selModel(&entryModel, this);
  Tellico::EntrySelectionModel proxySelect(&iconModel, &selModel, this);

//  connect(proxySelect, SIGNAL(entriesSelected(Tellico::Data::EntryList)),
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default fields
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QStringLiteral("title"), QStringLiteral("test1"));
  coll->addEntries(entry1);

  entryModel.setFields(coll->fields());
  entryModel.setEntries(coll->entries());

  QSignalSpy entriesSelectedSpy(&proxySelect, SIGNAL(entriesSelected(Tellico::Data::EntryList)));
  selModel.select(entryModel.index(0,0), QItemSelectionModel::Select);
  QCOMPARE(entriesSelectedSpy.count(), 1);

  Tellico::Data::EntryList entries = proxySelect.selectedEntries();
  QCOMPARE(entries.count(), 1);
  QCOMPARE(entries.at(0)->title(), QStringLiteral("test1"));

  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  entry2->setField(QStringLiteral("title"), QStringLiteral("test2"));
  coll->addEntries(entry2);
  entryModel.addEntries(Tellico::Data::EntryList() << entry2);

  entries = proxySelect.selectedEntries();
  QCOMPARE(entries.count(), 1);
  QCOMPARE(entries.at(0)->title(), QStringLiteral("test1"));

  selModel.select(entryModel.index(1,0), QItemSelectionModel::Select);
  entries = proxySelect.selectedEntries();
  QCOMPARE(entries.count(), 2);
  QCOMPARE(entries.at(0)->title(), QStringLiteral("test1"));
  QCOMPARE(entries.at(1)->title(), QStringLiteral("test2"));

  selModel.select(entryModel.index(0,0), QItemSelectionModel::Toggle);
  entries = proxySelect.selectedEntries();
  QCOMPARE(entries.count(), 1);
  QCOMPARE(entries.at(0)->title(), QStringLiteral("test2"));

  entryModel.clear();
  // now there should be no selection since EntryModel::clear calls modelReset()
  // and the proxy selection is connected to that signal
  entries = proxySelect.selectedEntries();
  QCOMPARE(entries.count(), 0);
}
