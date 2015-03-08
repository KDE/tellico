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

#include "z3950fetchertest.h"
#include "qtest_kde.h"

#include "../fetch/z3950fetcher.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"

#include <KStandardDirs>

QTEST_KDEMAIN( Z3950FetcherTest, GUI )

Z3950FetcherTest::Z3950FetcherTest() : AbstractFetcherTest() {
}

void Z3950FetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBibtex(Tellico::Data::Collection::Book, "book");
  // since we use the MODS importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  // also use the z3950-servers.cfg file
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../fetch/");
}

void Z3950FetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QLatin1String("Foundations of Qt Development"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::Z3950Fetcher(this, QLatin1String("loc")));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Foundations of Qt development"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Thelin, Johan."));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("1-59059-831-8"));
}

void Z3950FetcherTest::testIsbn() {
  // also testing multiple values
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("978-1-59059-831-3; 0-201-88954-4"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::Z3950Fetcher(this, QLatin1String("loc")));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 2);

  Tellico::Data::EntryPtr entry = results.at(1);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Foundations of Qt development"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Thelin, Johan."));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("1-59059-831-8"));
}

void Z3950FetcherTest::testADS() {
  // ADS has disappeared with no warning
  return;
  // also testing multiple values
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Raw,
                                       QLatin1String("@and @attr 1=4 \"Particle creation by black holes\" "
                                                     "@and @attr 1=1003 Hawking @attr 1=62 Generalized"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::Z3950Fetcher(this,
                                                                        QLatin1String("z3950.adsabs.harvard.edu"),
                                                                        210,
                                                                        QLatin1String("AST"),
                                                                        QLatin1String("ads")));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Particle creation by black holes"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Hawking, S. W."));
  QCOMPARE(entry->field(QLatin1String("year")), QLatin1String("1975"));
  QCOMPARE(entry->field(QLatin1String("doi")), QLatin1String("10.1007/BF02345020"));
  QCOMPARE(entry->field(QLatin1String("pages")), QLatin1String("199-220"));
  QCOMPARE(entry->field(QLatin1String("volume")), QLatin1String("43"));
  QCOMPARE(entry->field(QLatin1String("journal")), QLatin1String("Communications In Mathematical Physics"));
  QVERIFY(!entry->field(QLatin1String("url")).isEmpty());
}
