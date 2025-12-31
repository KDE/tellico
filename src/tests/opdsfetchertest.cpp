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

void OPDSFetcherTest::testEmptyGutenberg() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("Feedbooks"));
  cg.writeEntry("Catalog", "https://m.gutenberg.org/ebooks.opds/");

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("XXXXXXXXXXXX"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OPDSFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  // should be no results
  QVERIFY(results.isEmpty());
  QVERIFY(!fetcher->attribution().isEmpty());
}

void OPDSFetcherTest::testRelativeSearch() {
  QUrl catalog = QUrl::fromLocalFile(QFINDTESTDATA("data/opds-relative-search.xml"));
  QUrl search = catalog.resolved(QUrl(QStringLiteral("../opensearch.xml")));
  Tellico::Fetch::OPDSFetcher::Reader reader(catalog);

  QVERIFY(reader.parse());
  QUrl searchUrl = reader.searchUrl;
  QVERIFY(!searchUrl.isEmpty());
  QVERIFY(!searchUrl.isRelative());
  QCOMPARE(searchUrl.url(), search.url());
}

void OPDSFetcherTest::testAcquisitionByTitle() {
  QUrl catalog = QUrl::fromLocalFile(QFINDTESTDATA("data/opds-acquisition.xml"));
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("Feedbooks"));
  cg.writeEntry("Catalog", catalog.url());

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("Pride and Prejudice"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OPDSFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("title"), "Pride and Prejudice");
  QCOMPARE(entry->field("author"), "Jane Austen");
  QCOMPARE(entry->field("pub_year"), "1813");
  QCOMPARE(entry->field("genre"), "Fiction; Romance");
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field("cover").contains(QLatin1String("..")));
  QVERIFY(!entry->field("cover").contains(QLatin1Char('/')));
}

void OPDSFetcherTest::testAcquisitionByTitleNegative() {
  QUrl catalog = QUrl::fromLocalFile(QFINDTESTDATA("data/opds-acquisition.xml"));
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("Feedbooks"));
  cg.writeEntry("Catalog", catalog.url());

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("Nonexistent"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OPDSFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);
  QVERIFY(results.isEmpty());
}

void OPDSFetcherTest::testAcquisitionByKeyword() {
  QUrl catalog = QUrl::fromLocalFile(QFINDTESTDATA("data/opds-acquisition.xml"));
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("Feedbooks"));
  cg.writeEntry("Catalog", catalog.url());

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Keyword,
                                       QStringLiteral("fizzes"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OPDSFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 16);
}

void OPDSFetcherTest::testCalibre() {
  QUrl catalog = QUrl::fromLocalFile(QFINDTESTDATA("data/opds-calibre.xml"));
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("Calibre"));
  cg.writeEntry("Catalog", catalog.url());

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("1632"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OPDSFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("title"), "1632");
  QCOMPARE(entry->field("author"), "Flint, Eric");
  QCOMPARE(entry->field("rating"), "4");
  QCOMPARE(entry->field("series"), "1632");
  QCOMPARE(entry->field("series_num"), "1");
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("plot").contains("SUMMARY"));
}
