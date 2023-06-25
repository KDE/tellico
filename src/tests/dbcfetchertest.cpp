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

#include "dbcfetchertest.h"

#include "../fetch/dbcfetcher.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( DBCFetcherTest )

DBCFetcherTest::DBCFetcherTest() : AbstractFetcherTest() {
}

void DBCFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/dbc2tellico.xsl"));
}

void DBCFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       QStringLiteral("Min kamp"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DBCFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Min kamp. 6. bog"));
  QCOMPARE(entry->field(QStringLiteral("author")), QString::fromUtf8("Karl Ove Knausgård"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Lindhardt og Ringhof"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2012"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("978-87-11-39509-7"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("E-Book"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("barndomserindringer; erindringer"));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void DBCFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::ISBN,
                                       QStringLiteral("9788711391839"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DBCFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Min kamp. 1. bog"));
  QCOMPARE(entry->field(QStringLiteral("author")), QString::fromUtf8("Karl Ove Knausgård"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Lindhardt og Ringhof"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2012"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("978-87-11-39183-9"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("487"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("Dansk"));
  QCOMPARE(entry->field(QStringLiteral("translator")), QStringLiteral("Sara Koch"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("barndomserindringer; erindringer"));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void DBCFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Keyword,
                                       QStringLiteral("9788711391839"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DBCFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Min kamp. 1. bog"));
  QCOMPARE(entry->field(QStringLiteral("author")), QString::fromUtf8("Karl Ove Knausgård"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Lindhardt og Ringhof"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2012"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("978-87-11-39183-9"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("487"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("barndomserindringer; erindringer"));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}
