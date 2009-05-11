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

void TellicoReadTest::initTestCase() {
  // need to register this first
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
}

void TellicoReadTest::testLoad() {
  KUrl url1(QString::fromLatin1(KDESRCDIR) + "/data/books-format1.bc");
  KUrl url2(QString::fromLatin1(KDESRCDIR) + "/data/books-format2.bc");

  Tellico::Import::TellicoImporter importer1(url1);
  Tellico::Data::CollPtr coll1 = importer1.collection();

  Tellico::Import::TellicoImporter importer2(url2);
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(!coll1.isNull());
  QCOMPARE(coll1->type(), coll2->type());
  QCOMPARE(coll1->title(), coll2->title());
  QCOMPARE(coll1->entryCount(), coll2->entryCount());

  Tellico::Data::EntryPtr entry1 = coll1->entryById(0);
  Tellico::Data::EntryPtr entry2 = coll2->entryById(0);
  QCOMPARE(entry1->title(), entry2->title());
  QCOMPARE(entry1->field("author"), entry2->field("author"));
  QCOMPARE(entry1->field("keyword"), entry2->field("keyword"));
}
