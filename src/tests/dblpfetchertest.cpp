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
                                       QStringLiteral("Chip and PIN is Broken"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DBLPFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Chip and PIN is Broken."));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Steven J. Murdoch; Saar Drimer; Ross J. Anderson; Mike Bond"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2010"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("433-446"));
  QCOMPARE(entry->field(QStringLiteral("booktitle")), QStringLiteral("IEEE Symposium on Security and Privacy"));
  QCOMPARE(entry->field(QStringLiteral("journal")), QString());
  QCOMPARE(entry->field(QStringLiteral("url")), QStringLiteral("https://dblp.org/rec/conf/sp/MurdochDAB10"));
//  QCOMPARE(entry->field(QStringLiteral("doi")), QStringLiteral("10.1109/SP.2010.33"));
  QCOMPARE(entry->field(QStringLiteral("entry-type")), QStringLiteral("inproceedings"));
  QCOMPARE(entry->field(QStringLiteral("bibtex-key")), QStringLiteral("MurdochDAB10"));
}

void DBLPFetcherTest::testArticle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Keyword,
                                       QStringLiteral("Nontrivial independent sets of bipartite graphs"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DBLPFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Nontrivial independent sets of bipartite graphs and cross-intersecting families."));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Jun Wang 0132; Huajun Zhang 0005"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2013"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("129-141"));
  QCOMPARE(entry->field(QStringLiteral("volume")), QStringLiteral("120"));
  QCOMPARE(entry->field(QStringLiteral("number")), QStringLiteral("1"));
  QCOMPARE(entry->field(QStringLiteral("journal")), QStringLiteral("J. Comb. Theory A"));
  QCOMPARE(entry->field(QStringLiteral("booktitle")), QString());
  QCOMPARE(entry->field(QStringLiteral("url")), QStringLiteral("https://dblp.org/rec/journals/jct/WangZ13"));
//  QCOMPARE(entry->field(QStringLiteral("doi")), QStringLiteral("10.1016/j.jcta.2012.07.005"));
  QCOMPARE(entry->field(QStringLiteral("entry-type")), QStringLiteral("article"));
  QCOMPARE(entry->field(QStringLiteral("bibtex-key")), QStringLiteral("WangZ13"));
}
