/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#include "adsfetchertest.h"

#include "../fetch/adsfetcher.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"

#include <QTest>

QTEST_GUILESS_MAIN( ADSFetcherTest )

ADSFetcherTest::ADSFetcherTest() : AbstractFetcherTest() {
}

void ADSFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBib(Tellico::Data::Collection::Bibtex, "entry");
}

void ADSFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       QStringLiteral("spacecraft architectures for the Terrestrial Planet Finder"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ADSFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("title"), "Comparison of structurally connected and separated spacecraft architectures for the Terrestrial Planet Finder");
  QCOMPARE(entry->field("author"), "Stephenson, Robert L.; Miller, David W.");
  QCOMPARE(entry->field("entry-type"), "inproceedings");
  QCOMPARE(entry->field("year"), "1998");
  QCOMPARE(entry->field("pages"), "672-682");
  QCOMPARE(entry->field("volume"), "3350");
  QCOMPARE(entry->field("journal"), "Astronomical Interferometry");
  QCOMPARE(entry->field("doi"), "10.1117/12.317131");
  QCOMPARE(entry->field("url"), "https://ui.adsabs.harvard.edu/abs/1998SPIE.3350..672S");
  QVERIFY(!entry->field("abstract").isEmpty());
}

void ADSFetcherTest::testAuthor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Person,
                                       QStringLiteral("Miret-Roig, Núria"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ADSFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);
  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  // mangles the accent
//  QVERIFY(entry->field("author").contains("Miret-Roig, Núria"));
  QVERIFY(entry->field("author").contains("Miret-Roig, Nuria"));
}

void ADSFetcherTest::testDOI() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::DOI,
                                       QLatin1String("10.1117/12.317131"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ADSFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);
  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("title"), "Comparison of structurally connected and separated spacecraft architectures for the Terrestrial Planet Finder");
  QCOMPARE(entry->field("author"), "Stephenson, Robert L.; Miller, David W.");
  QCOMPARE(entry->field("entry-type"), "inproceedings");
  QCOMPARE(entry->field("year"), "1998");
  QCOMPARE(entry->field("pages"), "672-682");
  QCOMPARE(entry->field("volume"), "3350");
  QCOMPARE(entry->field("journal"), "Astronomical Interferometry");
  QCOMPARE(entry->field("doi"), "10.1117/12.317131");
  QCOMPARE(entry->field("url"), "https://ui.adsabs.harvard.edu/abs/1998SPIE.3350..672S");
  QVERIFY(!entry->field("abstract").isEmpty());
}
