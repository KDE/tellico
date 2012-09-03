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

#undef QT_NO_CAST_FROM_ASCII

#include "delicioustest.h"
#include "delicioustest.moc"
#include "qtest_kde.h"

#include "../translators/deliciousimporter.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../fieldformat.h"

#include <kstandarddirs.h>

#define FIELDS(entry, fieldName) Tellico::FieldFormat::splitValue(entry->field(fieldName))

QTEST_KDEMAIN_CORE( DeliciousTest )

void DeliciousTest::initTestCase() {
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
}

void DeliciousTest::testBooks1() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/delicious1_books.xml");
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 5);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("Lost in Translation"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("1998"));
  QCOMPARE(entry->field("author"), QLatin1String("Nicole Mones; Robby Stephenson"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Delacorte Press"));
  QCOMPARE(entry->field("isbn"), QLatin1String("0385319347"));
  QCOMPARE(entry->field("binding"), QLatin1String("Hardback"));
  QCOMPARE(entry->field("genre"), QLatin1String("United States; Contemporary & Robby"));
  QCOMPARE(entry->field("pages"), QLatin1String("384"));
  QCOMPARE(entry->field("rating"), QLatin1String("4"));
  QCOMPARE(entry->field("pur_price"), QLatin1String("$23.95"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("07-08-2006"));
  QVERIFY(entry->field("comments").startsWith(QLatin1String("<p><span style=\"font-size:12pt;\">Nicole Mones doesn't")));
}

void DeliciousTest::testBooks2() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/delicious2_books.xml");
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 7);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("The Restaurant at the End of the Universe"));
  QCOMPARE(entry->field("isbn"), QLatin1String("0517545357"));
  QCOMPARE(entry->field("cdate"), QLatin1String("2007-12-19"));
  QCOMPARE(entry->field("mdate"), QLatin1String("2009-06-11"));
  QCOMPARE(FIELDS(entry, "author").count(), 1);
  QCOMPARE(FIELDS(entry, "author").first(), QLatin1String("Douglas Adams"));
  QCOMPARE(entry->field("binding"), QLatin1String("Hardback"));
  QCOMPARE(entry->field("rating"), QLatin1String("4.5")); // visually, this gets shown as 4 stars
  QCOMPARE(entry->field("pages"), QLatin1String("250"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("1982"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Harmony"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("2007-12-18"));
  QCOMPARE(entry->field("pur_price"), QLatin1String("$12.95"));
  QCOMPARE(entry->field("signed"), QLatin1String("true"));
  QCOMPARE(entry->field("condition"), QLatin1String("Used"));
}
