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

#include "opdsfetchertest.h"

#include "../fetch/opdsfetcher.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( OPDSFetcherTest )

OPDSFetcherTest::OPDSFetcherTest() : AbstractFetcherTest() {
  QStandardPaths::setTestModeEnabled(true);
}

void OPDSFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/atom2tellico.xsl"));
  Tellico::ImageFactory::init();
}

void OPDSFetcherTest::testFeedbooksSearch() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group("Feedbooks");
  cg.writeEntry("Catalog", "https://www.feedbooks.com/catalog.atom");
  cg.writeEntry("Custom Fields", "url");

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       "9781773231341");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OPDSFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("title"), "First Lensman");
  QCOMPARE(entry->field("author"), "E. E. Smith");
  QCOMPARE(entry->field("isbn"), "978-1-77323-134-1");
  QCOMPARE(entry->field("pub_year"), "2018");
  QCOMPARE(entry->field("publisher"), "Reading Essentials");
  QCOMPARE(entry->field("genre"), "Fiction; Science fiction; Space opera and planet opera");
  QCOMPARE(entry->field("pages"), "226");
  QCOMPARE(entry->field("url"), "https://www.feedbooks.com/item/2971293");
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field("cover").contains(QLatin1Char('/')));
  QVERIFY(!entry->field("plot").isEmpty());
}

void OPDSFetcherTest::testEmptyGutenberg() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group("Feedbooks");
  cg.writeEntry("Catalog", "https://m.gutenberg.org/ebooks.opds/");

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       "XXXXXXXXXXXX");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OPDSFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  // should be no results
  QVERIFY(results.isEmpty());
  QVERIFY(!fetcher->attribution().isEmpty());
}

void OPDSFetcherTest::testRelativeSearch() {
  QUrl catalog = QUrl::fromLocalFile(QFINDTESTDATA("data/opds-relative-search.xml"));
  QUrl search = catalog.resolved(QUrl(QLatin1String("../opensearch.xml")));
  Tellico::Fetch::OPDSFetcher::Reader reader(catalog);

  QVERIFY(reader.parse());
  QUrl searchUrl = reader.searchUrl;
  QVERIFY(!searchUrl.isEmpty());
  QVERIFY(!searchUrl.isRelative());
  QCOMPARE(searchUrl.url(), search.url());
}

void OPDSFetcherTest::testAcquisitionByTitle() {
  QUrl catalog = QUrl::fromLocalFile(QFINDTESTDATA("data/opds-acquisition.xml"));
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group("Feedbooks");
  cg.writeEntry("Catalog", catalog.url());

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       "Pride and Prejudice");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OPDSFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("title"), "Pride and Prejudice");
  QCOMPARE(entry->field("author"), "Jane Austen");
  QCOMPARE(entry->field("pub_year"), "1813");
  QCOMPARE(entry->field("genre"), "Fiction; Romance");
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field("cover").contains(QLatin1Char('/')));
  QVERIFY(!entry->field("cover").contains(QLatin1Char('.')));
}

void OPDSFetcherTest::testAcquisitionByTitleNegative() {
  QUrl catalog = QUrl::fromLocalFile(QFINDTESTDATA("data/opds-acquisition.xml"));
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group("Feedbooks");
  cg.writeEntry("Catalog", catalog.url());

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       "Nonexistent");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OPDSFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);
  QVERIFY(results.isEmpty());
}

void OPDSFetcherTest::testAcquisitionByKeyword() {
  QUrl catalog = QUrl::fromLocalFile(QFINDTESTDATA("data/opds-acquisition.xml"));
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group("Feedbooks");
  cg.writeEntry("Catalog", catalog.url());

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Keyword,
                                       "fizzes");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OPDSFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 16);
}
