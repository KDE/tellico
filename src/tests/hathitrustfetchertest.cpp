/***************************************************************************
    Copyright (C) 2015 Robby Stephenson <robby@periapsis.org>
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

#include "hathitrustfetchertest.h"

#include "../fetch/hathitrustfetcher.h"
#include "../entry.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( HathiTrustFetcherTest )

HathiTrustFetcherTest::HathiTrustFetcherTest() : AbstractFetcherTest() {
}

void HathiTrustFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/MARC21slim2MODS3.xsl"));
}

void HathiTrustFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("1931561567"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::HathiTrustFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  compareEntry(entry);
}

void HathiTrustFetcherTest::testLccn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::LCCN,
                                       QLatin1String("2003019402"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::HathiTrustFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  compareEntry(entry);
}

void HathiTrustFetcherTest::compareEntry(Tellico::Data::EntryPtr entry) {
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("The forgotten genius"));
  QCOMPARE(entry->field(QLatin1String("subtitle")), QLatin1String("the biography of Robert Hooke 1635-1703"));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("1-931561-56-7"));
  QCOMPARE(entry->field(QLatin1String("lccn")), QLatin1String("2003019402"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Inwood, Stephen"));
  QCOMPARE(entry->field(QLatin1String("publisher")), QLatin1String("MacAdam/Cage Pub."));
  QCOMPARE(entry->field(QLatin1String("pub_year")), QLatin1String("2003"));
  QCOMPARE(entry->field(QLatin1String("genre")), QLatin1String("bibliography; biography"));
  QVERIFY(entry->field(QLatin1String("keyword")).contains(QLatin1String("Architecture")));
  QVERIFY(entry->field(QLatin1String("keyword")).contains(QLatin1String("history")));
  QVERIFY(entry->field(QLatin1String("comments")).contains(QLatin1String("London")));
}
