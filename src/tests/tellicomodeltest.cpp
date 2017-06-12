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
#include "../models/entrysortmodel.h"
#include "../models/filtermodel.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../document.h"

#include <QTest>

QTEST_GUILESS_MAIN( TellicoModelTest )

void TellicoModelTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
}

void TellicoModelTest::testEntryModel() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default fields
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QLatin1String("title"), QLatin1String("Star Wars"));
  coll->addEntries(entry1);

  Tellico::EntryModel entryModel(this);
  ModelTest test1(&entryModel);

  Tellico::EntrySortModel sortModel(this);
  ModelTest test2(&sortModel);

  sortModel.setSourceModel(&entryModel);

  entryModel.setFields(coll->fields());
  entryModel.setEntries(coll->entries());
  QCOMPARE(entryModel.index(0, 0), entryModel.indexFromEntry(entry1));

  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(QLatin1String("test"), QLatin1String("test")));
  coll->addField(field1);
  entryModel.setFields(coll->fields());

  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  coll->addEntries(entry2);
  entryModel.addEntries(Tellico::Data::EntryList() << entry2);

  Tellico::Data::FieldPtr field2(new Tellico::Data::Field(QLatin1String("test2"), QLatin1String("test2")));
  coll->addField(field2);
  entryModel.addFields(Tellico::Data::FieldList() << field2);

  Tellico::FilterRule* rule1 = new Tellico::FilterRule(QLatin1String("title"),
                                                       QLatin1String("Star Wars"),
                                                       Tellico::FilterRule::FuncEquals);
  Tellico::FilterPtr filter(new Tellico::Filter(Tellico::Filter::MatchAny));
  filter->append(rule1);

  sortModel.setFilter(filter);

  entryModel.clear();
  entryModel.setFields(coll->fields());
  entryModel.setEntries(coll->entries());
  QCOMPARE(entryModel.index(0, 0), entryModel.indexFromEntry(entry1));

  Tellico::Data::FieldPtr field3(new Tellico::Data::Field(QLatin1String("test"), QLatin1String("test-new")));
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
}

void TellicoModelTest::testFilterModel() {
  Tellico::FilterModel filterModel(this);
  ModelTest test1(&filterModel);

  Tellico::FilterRule* rule1 = new Tellico::FilterRule(QLatin1String("title"),
                                                       QLatin1String("Star Wars"),
                                                       Tellico::FilterRule::FuncEquals);
  Tellico::FilterPtr filter(new Tellico::Filter(Tellico::Filter::MatchAny));
  filter->append(rule1);
  filterModel.addFilter(filter);

  Tellico::Data::CollPtr c = Tellico::Data::Document::self()->collection();
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(c));
  entry1->setField(QLatin1String("title"), QLatin1String("Star Wars"));
  c->addEntries(entry1);

  filterModel.clear();
  filterModel.addFilter(filter);
  QCOMPARE(filter, filterModel.filter(filterModel.index(0, 0)));
  QVERIFY(filterModel.indexContainsEntry(filterModel.index(0, 0), entry1));

  filterModel.invalidate(filterModel.index(0, 0));
  QCOMPARE(filter, filterModel.filter(filterModel.index(0, 0)));
  QVERIFY(filterModel.indexContainsEntry(filterModel.index(0, 0), entry1));
}
