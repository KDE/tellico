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

#include "openlibraryfetchertest.h"
#include "qtest_kde.h"

#include "../fetch/openlibraryfetcher.h"
#include "../entry.h"
#include "../images/imagefactory.h"

QTEST_KDEMAIN( OpenLibraryFetcherTest, GUI )

OpenLibraryFetcherTest::OpenLibraryFetcherTest() : AbstractFetcherTest() {
}

void OpenLibraryFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void OpenLibraryFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("0789312239"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("This is Venice"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("M. Sasek"));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("0-7893-1223-9"));
  QCOMPARE(entry->field(QLatin1String("lccn")), QLatin1String("2004110229"));
  QCOMPARE(entry->field(QLatin1String("pub_year")), QLatin1String("2005"));
  QCOMPARE(entry->field(QLatin1String("genre")), QLatin1String("Juvenile literature."));
  QCOMPARE(entry->field(QLatin1String("keyword")), QLatin1String("Venice (Italy) -- Description and travel -- Juvenile literature"));
  QCOMPARE(entry->field(QLatin1String("publisher")), QLatin1String("Universe"));
  QCOMPARE(entry->field(QLatin1String("language")), QLatin1String("English"));
  QCOMPARE(entry->field(QLatin1String("pages")), QLatin1String("56"));
  QVERIFY(!entry->field(QLatin1String("comments")).isEmpty());
}

void OpenLibraryFetcherTest::testIsbn13() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("9780596004361"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("C Pocket Reference"));
}

void OpenLibraryFetcherTest::testMultipleIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("0789312239; 9780596000486"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 2);
}
