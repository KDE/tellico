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

#include "dblpfetchertest.h"

#include "../fetch/dblpfetcher.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( DBLPFetcherTest )

DBLPFetcherTest::DBLPFetcherTest() : AbstractFetcherTest() {
}

void DBLPFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/dblp2tellico.xsl"));
}

void DBLPFetcherTest::testProceedings() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Keyword,
                                       QLatin1String("Chip and PIN is Broken"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DBLPFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Chip and PIN is Broken."));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Steven J. Murdoch; Saar Drimer; Ross J. Anderson; Mike Bond"));
  QCOMPARE(entry->field(QLatin1String("year")), QLatin1String("2010"));
  QCOMPARE(entry->field(QLatin1String("pages")), QLatin1String("433-446"));
  QCOMPARE(entry->field(QLatin1String("booktitle")), QLatin1String("IEEE Symposium on Security and Privacy"));
  QCOMPARE(entry->field(QLatin1String("journal")), QLatin1String(""));
  QCOMPARE(entry->field(QLatin1String("url")), QLatin1String("http://dblp.org/rec/conf/sp/MurdochDAB10"));
//  QCOMPARE(entry->field(QLatin1String("doi")), QLatin1String("10.1109/SP.2010.33"));
  QCOMPARE(entry->field(QLatin1String("entry-type")), QLatin1String("inproceedings"));
  QCOMPARE(entry->field(QLatin1String("bibtex-key")), QLatin1String("MurdochDAB10"));
}

void DBLPFetcherTest::testArticle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Keyword,
                                       QLatin1String("Nontrivial independent sets of bipartite graphs"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DBLPFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Nontrivial independent sets of bipartite graphs and cross-intersecting families."));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Jun Wang; Huajun Zhang"));
  QCOMPARE(entry->field(QLatin1String("year")), QLatin1String("2013"));
  QCOMPARE(entry->field(QLatin1String("pages")), QLatin1String("129-141"));
  QCOMPARE(entry->field(QLatin1String("volume")), QLatin1String("120"));
  QCOMPARE(entry->field(QLatin1String("number")), QLatin1String("1"));
  QCOMPARE(entry->field(QLatin1String("journal")), QLatin1String("J. Comb. Theory, Ser. A"));
  QCOMPARE(entry->field(QLatin1String("booktitle")), QLatin1String(""));
  QCOMPARE(entry->field(QLatin1String("url")), QLatin1String("http://dblp.org/rec/journals/jct/WangZ13"));
//  QCOMPARE(entry->field(QLatin1String("doi")), QLatin1String("10.1016/j.jcta.2012.07.005"));
  QCOMPARE(entry->field(QLatin1String("entry-type")), QLatin1String("article"));
  QCOMPARE(entry->field(QLatin1String("bibtex-key")), QLatin1String("WangZ13"));
}
