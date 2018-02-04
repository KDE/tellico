/***************************************************************************
    Copyright (C) 2009-2011 Robby Stephenson <robby@periapsis.org>
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

#include "isbndbfetchertest.h"

#include "../fetch/isbndbfetcher.h"
#include "../entry.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( ISBNdbFetcherTest )

ISBNdbFetcherTest::ISBNdbFetcherTest() : AbstractFetcherTest() {
}

void ISBNdbFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/isbndb2tellico.xsl"));
}

void ISBNdbFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0789312239"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")).toLower(), QStringLiteral("this is venice"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Miroslav Sasek"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-7893-1223-9"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2005"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Universe"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Hardback"));
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
}

void ISBNdbFetcherTest::testIsbn13() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("9780596004361"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("C Pocket Reference"));
}

void ISBNdbFetcherTest::testMultipleIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0789312239; 9780596000486"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QEXPECT_FAIL("", "ISBNdb fetcher does not yet support searching for multiple ISBNs", Continue);
  QCOMPARE(results.size(), 2);
}

void ISBNdbFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("PACKING FOR MARS The Curious Science of Life in the Void"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  static_cast<Tellico::Fetch::ISBNdbFetcher*>(fetcher.data())->setLimit(1);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry= results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("PACKING FOR MARS The Curious Science of Life in the Void"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Roach, Mary"));
  QCOMPARE(entry->field(QStringLiteral("isbn")).remove('-'), QStringLiteral("1611293758"));
//  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2010"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("W. W. Norton & Co"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
}

void ISBNdbFetcherTest::testAuthor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Person,
                                       QStringLiteral("Joshua Foer"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  static_cast<Tellico::Fetch::ISBNdbFetcher*>(fetcher.data())->setLimit(1);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry= results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Moonwalking with Einstein"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Joshua Foer"));
  QCOMPARE(entry->field(QStringLiteral("isbn")).remove('-'), QStringLiteral("0143120530"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2012"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Penguin (Non-Classics)"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
}
