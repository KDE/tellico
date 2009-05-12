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
#include "tellicoreadtest.h"
#include "tellicoreadtest.moc"

#include "../translators/tellicoimporter.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"

QTEST_KDEMAIN_CORE( TellicoReadTest )

#define QL1(x) QString::fromLatin1(x)

void TellicoReadTest::initTestCase() {
  // need to register this first
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");

  for(int i = 1; i < 6; ++i) {
    KUrl url(QL1(KDESRCDIR) + QL1("/data/books-format%1.bc").arg(i));

    Tellico::Import::TellicoImporter importer(url);
    Tellico::Data::CollPtr coll = importer.collection();
    m_collections.append(coll);
  }
}

void TellicoReadTest::testCollection() {
  Tellico::Data::CollPtr coll1 = m_collections[0];
  // skip the first one
  for(int i = 1; i < m_collections.count(); ++i) {
    Tellico::Data::CollPtr coll2 = m_collections[i];
    QVERIFY(!coll2.isNull());
    QCOMPARE(coll1->type(), coll2->type());
    QCOMPARE(coll1->title(), coll2->title());
    QCOMPARE(coll1->entryCount(), coll2->entryCount());
  }
}

void TellicoReadTest::testEntries() {
  QFETCH(QString, fieldName);

  Tellico::Data::FieldPtr field1 = m_collections[0]->fieldByName(fieldName);
  Tellico::Data::EntryPtr entry1 = m_collections[0]->entryById(0);

  // skip the first one
  for(int i = 1; i < m_collections.count(); ++i) {
    Tellico::Data::FieldPtr field2 = m_collections[i]->fieldByName(fieldName);
    if(field1 && field2) {
      QCOMPARE(field1->name(), field2->name());
      QCOMPARE(field1->title(), field2->title());
      QCOMPARE(field1->category(), field2->category());
      QCOMPARE(field1->type(), field2->type());
      QCOMPARE(field1->flags(), field2->flags());
    }

    Tellico::Data::EntryPtr entry2 = m_collections[i]->entryById(0);
    QCOMPARE(entry1->field(fieldName), entry2->field(fieldName));
  }
}

void TellicoReadTest::testEntries_data() {
  QTest::addColumn<QString>("fieldName");

  QTest::newRow("title") << QL1("title");
  QTest::newRow("author") << QL1("author");
  QTest::newRow("keywords") << QL1("keywords");
  QTest::newRow("keyword") << QL1("keyword");
  QTest::newRow("genre") << QL1("genre");
  QTest::newRow("isbn") << QL1("isbn");
  QTest::newRow("rating") << QL1("rating");
  QTest::newRow("comments") << QL1("comments");
}
