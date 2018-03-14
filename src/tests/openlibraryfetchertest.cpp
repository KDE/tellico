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

#include "../fetch/openlibraryfetcher.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <QTest>

QTEST_GUILESS_MAIN( OpenLibraryFetcherTest )

OpenLibraryFetcherTest::OpenLibraryFetcherTest() : AbstractFetcherTest() {
}

void OpenLibraryFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void OpenLibraryFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0789312239"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("This is Venice"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("M. Sasek"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-7893-1223-9"));
  QCOMPARE(entry->field(QStringLiteral("lccn")), QStringLiteral("2004110229"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2005"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Juvenile literature."));
  QCOMPARE(entry->field(QStringLiteral("keyword")), QStringLiteral("Venice (Italy) -- Description and travel -- Juvenile literature"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Universe"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("English"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("56"));
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
}

void OpenLibraryFetcherTest::testIsbn13() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("9780596004361"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("C Pocket Reference"));
}

void OpenLibraryFetcherTest::testMultipleIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0789312239; 9780596000486"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 2);
}
