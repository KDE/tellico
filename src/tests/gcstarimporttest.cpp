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
#include "gcstarimporttest.h"
#include "gcstarimporttest.moc"

#include "../translators/gcstarimporter.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN_CORE( GCstarImportTest )

void GCstarImportTest::initTestCase() {
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
}

void GCstarImportTest::testImport() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test.gcs");
  Tellico::Import::GCstarImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 1);
  // should be translated somehow
  QCOMPARE(coll->title(), QLatin1String("GCstar Import"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("The Reason for God"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("2008"));
  QCOMPARE(entry->fields("author", false).count(), 2);
  QCOMPARE(entry->fields("author", false).first(), QLatin1String("Timothy Keller"));
  QCOMPARE(entry->field("isbn"), QLatin1String("978-0-525-95049-3"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Dutton Adult"));
  QCOMPARE(entry->fields("genre", false).count(), 2);
  QCOMPARE(entry->fields("genre", false).at(0), QLatin1String("non-fiction"));
  QCOMPARE(entry->fields("keyword", false).count(), 2);
  QCOMPARE(entry->fields("keyword", false).at(0), QLatin1String("tag1"));
  QCOMPARE(entry->fields("keyword", false).at(1), QLatin1String("tag2"));
  // file has rating of 4, Tellico uses half the rating of GCstar, so it should be 2
  QCOMPARE(entry->field("rating"), QLatin1String("2"));
}
