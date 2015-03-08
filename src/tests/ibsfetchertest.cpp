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

#include "ibsfetchertest.h"
#include "qtest_kde.h"

#include "../fetch/ibsfetcher.h"
#include "../entry.h"
#include "../images/imagefactory.h"

QTEST_KDEMAIN( IBSFetcherTest, GUI )

IBSFetcherTest::IBSFetcherTest() : AbstractFetcherTest() {
}

void IBSFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void IBSFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QLatin1String("Vino & cucina"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IBSFetcher(this));

  // the one we want should be in the first 5
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 5);

  QVERIFY(results.size() > 0);
  Tellico::Data::EntryPtr entry;  //  results can be randomly ordered, loop until we find the one we want
  foreach(Tellico::Data::EntryPtr testEntry, results) {
    if(testEntry->field(QLatin1String("isbn")) == QLatin1String("9788804620372")) {
      entry = testEntry;
      break;
    }
  }
  QVERIFY(entry);

  compareEntry(entry);
}

void IBSFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("9788804620372"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IBSFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  compareEntry(results.first());
}

void IBSFetcherTest::testAuthor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Person,
                                       QLatin1String("Bruno Vespa"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IBSFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 10);
  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr testEntry, results) {
    if(testEntry->field(QLatin1String("isbn")) == QLatin1String("9788804620372")) {
      entry = testEntry;
      break;
    }
  }
  QVERIFY(entry);
  compareEntry(entry);
}

void IBSFetcherTest::testTranslator() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("9788842914976"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IBSFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.first();
  QCOMPARE(entry->field("title"), QString::fromUtf8("La città sepolta"));
  QCOMPARE(entry->field("isbn"), QLatin1String("9788842914976"));
  QCOMPARE(entry->field("author"), QLatin1String("James Rollins"));
  QCOMPARE(entry->field("translator"), QLatin1String("M. Zonetti"));
}

void IBSFetcherTest::testEditor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("9788873718734"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IBSFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.first();
  QCOMPARE(entry->field("title"), QString::fromUtf8("La centuria bianca"));
  QCOMPARE(entry->field("editor"), QLatin1String("I. Armaro"));
  QCOMPARE(entry->field("binding"), QLatin1String("Paperback"));
}

void IBSFetcherTest::compareEntry(Tellico::Data::EntryPtr entry) {
  QVERIFY(entry->field("title").startsWith(QLatin1String("Vino & cucina")));
  QCOMPARE(entry->field("isbn"), QLatin1String("9788804620372"));
  QCOMPARE(entry->field("author"), QLatin1String("Antonella Clerici; Bruno Vespa"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("2012"));
  QCOMPARE(entry->field("pages"), QLatin1String("225"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Mondadori"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
}
