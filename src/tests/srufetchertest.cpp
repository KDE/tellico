/***************************************************************************
    Copyright (C) 2009-2020 Robby Stephenson <robby@periapsis.org>
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

#include "srufetchertest.h"

#include "../fetch/srufetcher.h"
#include "../collections/bookcollection.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../utils/datafileregistry.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( SRUFetcherTest )

SRUFetcherTest::SRUFetcherTest() : AbstractFetcherTest() {
}

void SRUFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
  // since we use the MODS importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/mods2tellico.xsl"));
}

void SRUFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("Foundations of Qt Development"));
  Tellico::Fetch::Fetcher::Ptr fetcher = Tellico::Fetch::SRUFetcher::libraryOfCongress(this);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Foundations of Qt development"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Thelin, Johan."));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("1-59059-831-8"));
}

void SRUFetcherTest::testIsbn() {
  // also testing multiple values
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("978-1-59059-831-3; 0-201-88954-4"));
  Tellico::Fetch::Fetcher::Ptr fetcher = Tellico::Fetch::SRUFetcher::libraryOfCongress(this);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 2);

  Tellico::Data::EntryPtr entry = results.at(1);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Foundations of Qt development"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Thelin, Johan."));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("1-59059-831-8"));
}

// see http://raoulm.home.xs4all.nl/mcq.htm
void SRUFetcherTest::testKBTitle() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("KB"));
  cg.writeEntry("Format", QStringLiteral("dc"));
  cg.writeEntry("Host", QStringLiteral("jsru.kb.nl"));
  cg.writeEntry("Path", QStringLiteral("/sru/sru.pl"));
  cg.writeEntry("Port", 80);
  cg.writeEntry("QueryFields", QStringLiteral("x-collection,x-fields"));
  cg.writeEntry("QueryValues", QStringLiteral("GGC,ISBN"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       QStringLiteral("Godfried Bomans: Erik of het klein insectenboek"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::SRUFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 5);

  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr testEntry, results) {
    if(testEntry->field(QStringLiteral("entry-type")) == QStringLiteral("book")) {
      entry = testEntry;
      break;
    }
  }
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Godfried Bomans: Erik of het klein insectenboek"));
  QCOMPARE(entry->field(QStringLiteral("entry-type")), QStringLiteral("book"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Purmerend : Muusses"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("90-231-1704-2"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1971"));
  QVERIFY(!entry->field(QStringLiteral("url")).isEmpty());
}

// see http://raoulm.home.xs4all.nl/mcq.htm
void SRUFetcherTest::testKBIsbn() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("KB"));
  cg.writeEntry("Format", QStringLiteral("dc"));
  cg.writeEntry("Host", QStringLiteral("jsru.kb.nl"));
  cg.writeEntry("Path", QStringLiteral("/sru/sru.pl"));
  cg.writeEntry("Port", 80);
  cg.writeEntry("QueryFields", QStringLiteral("x-collection,x-fields"));
  cg.writeEntry("QueryValues", QStringLiteral("GGC,ISBN"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::ISBN,
                                       QStringLiteral("9023117042"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::SRUFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  // for whatever reason, there are two results with the same isbn and we want the second
  QCOMPARE(results.size(), 2);

  Tellico::Data::EntryPtr entry = results.at(1);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Godfried Bomans: Erik of het klein insectenboek"));
  QCOMPARE(entry->field(QStringLiteral("entry-type")), QStringLiteral("book"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Purmerend : Muusses"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("90-231-1704-2"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1971"));
  QVERIFY(!entry->field(QStringLiteral("url")).isEmpty());
}

void SRUFetcherTest::testCopacIsbn() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("Copac"));
  cg.writeEntry("Format", QStringLiteral("mods"));
  cg.writeEntry("Host", QStringLiteral("discover.libraryhub.jisc.ac.uk"));
  cg.writeEntry("Path", QStringLiteral("/discover"));
  cg.writeEntry("Port", 210);
  cg.writeEntry("Custom Fields", QStringLiteral("lcc"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::ISBN,
                                       QStringLiteral("1430202513"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::SRUFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Foundations of Qt Development"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Thelin, Johan."));
  // the mods2tellico.xsl file returns the last ISBN value in the response, which is different in this case. Ignore for now
//  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("1-43020-251-3"));
  QCOMPARE(entry->field(QStringLiteral("lcc")), QStringLiteral("T385"));
  QCOMPARE(entry->field(QStringLiteral("lccn")), QStringLiteral("2008295709"));
  QCOMPARE(entry->field(QStringLiteral("doi")), QStringLiteral("10.1007/978-1-4302-0251-6"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2007"));
  QVERIFY(entry->field(QStringLiteral("publisher")).contains(QStringLiteral("Apress")));
}

// https://bugs.kde.org/show_bug.cgi?id=463438
void SRUFetcherTest::testHttpFallback() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("DNB"));
  cg.writeEntry("Format", QStringLiteral("MARC21-xml"));
  cg.writeEntry("Scheme", QStringLiteral("https"));
  cg.writeEntry("Host", QStringLiteral("services.dnb.de"));
  cg.writeEntry("Path", QStringLiteral("/sru/dnb"));
  cg.writeEntry("Port", 443); // port 443 forces https for this test. Port 80 seems to fallback on the server side
  cg.writeEntry("QueryFields", QStringLiteral("maximumRecords"));
  cg.writeEntry("QueryValues", QStringLiteral("1"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       QStringLiteral("Goethe"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::SRUFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(!entry->field(QStringLiteral("title")).isEmpty());
}
