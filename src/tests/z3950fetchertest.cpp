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

#include "../fetch/z3950fetcher.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( Z3950FetcherTest )

Z3950FetcherTest::Z3950FetcherTest() : AbstractFetcherTest() {
}

void Z3950FetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBibtex(Tellico::Data::Collection::Book, "book");
  // since we use the MODS importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/mods2tellico.xsl"));
  // also use the z3950-servers.cfg file
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../fetch/z3950-servers.cfg"));
}

void Z3950FetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("Foundations of Qt Development"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::Z3950Fetcher(this, QStringLiteral("loc")));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Foundations of Qt development"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Thelin, Johan."));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("1-59059-831-8"));
}

void Z3950FetcherTest::testIsbn() {
  // also testing multiple values
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("978-1-59059-831-3; 0-201-88954-4"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::Z3950Fetcher(this, QStringLiteral("loc")));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 2);

  Tellico::Data::EntryPtr entry = results.at(1);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Foundations of Qt development"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Thelin, Johan."));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("1-59059-831-8"));
}

void Z3950FetcherTest::testADS() {
  // 2014: ADS has disappeared with no warning
  return;
  // also testing multiple values
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Raw,
                                       QStringLiteral("@and @attr 1=4 \"Particle creation by black holes\" "
                                                      "@and @attr 1=1003 Hawking @attr 1=62 Generalized"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::Z3950Fetcher(this,
                                                                        QStringLiteral("z3950.adsabs.harvard.edu"),
                                                                        210,
                                                                        QStringLiteral("AST"),
                                                                        QStringLiteral("ads")));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Particle creation by black holes"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Hawking, S. W."));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1975"));
  QCOMPARE(entry->field(QStringLiteral("doi")), QStringLiteral("10.1007/BF02345020"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("199-220"));
  QCOMPARE(entry->field(QStringLiteral("volume")), QStringLiteral("43"));
  QCOMPARE(entry->field(QStringLiteral("journal")), QStringLiteral("Communications In Mathematical Physics"));
  QVERIFY(!entry->field(QStringLiteral("url")).isEmpty());
}

void Z3950FetcherTest::testBibsysIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("8242407665"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::Z3950Fetcher(this, QStringLiteral("bibsys")));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QString::fromUtf8("Grønn"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("82-42-40477-1"));
}

// https://bugs.kde.org/show_bug.cgi?id=419670
void Z3950FetcherTest::testPortugal() {
//  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::ISBN,
//                                       QString::fromUtf8("972-706-024-2"));
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       QStringLiteral("Memórias póstumas de Brás Cubas"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::Z3950Fetcher(this,
                                                                        QStringLiteral("z3950.porbase.bnportugal.gov.pt"),
                                                                        210,
                                                                        QStringLiteral("bn"),
                                                                        QStringLiteral("unimarc")));
  Tellico::Fetch::Z3950Fetcher* f = static_cast<Tellico::Fetch::Z3950Fetcher*>(fetcher.data());
  f->setCharacterSet(QStringLiteral("ISO-8859-1"), QStringLiteral("iso5426"));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")).toUtf8(), QString::fromUtf8("Memórias póstumas de Brás Cubas").toUtf8());
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Assis, Machado de"));
}
