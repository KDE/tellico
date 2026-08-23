/***************************************************************************
    Copyright (C) 2026 Robby Stephenson <robby@periapsis.org>
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

#include "goodreadsfetchertest.h"

#include "../fetch/goodreadsfetcher.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( GoodreadsFetcherTest )

GoodreadsFetcherTest::GoodreadsFetcherTest() : AbstractFetcherTest() {
}

void GoodreadsFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/goodreads2tellico.xsl"));
  Tellico::ImageFactory::init();

  m_config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("goodreads"));
  m_config.writeEntry("Custom Fields", QStringLiteral("goodreads"));
}

void GoodreadsFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("Ender's Game"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoodreadsFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Ender’s Game (Ender's Saga, #1)"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Orson Scott Card"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-8125-5070-6"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Tor"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2004"));
  QCOMPARE(entry->field(QStringLiteral("edition")), QStringLiteral("Author's Definitive Edition"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("324"));
  QCOMPARE(entry->field(QStringLiteral("goodreads")), QStringLiteral("https://www.goodreads.com/book/show/375802.Ender_s_Game"));
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void GoodreadsFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0-441172717"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoodreadsFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Dune (Dune, #1)"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Frank Herbert"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-441-17271-7"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Ace Books"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2020"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("884"));
  QCOMPARE(entry->field(QStringLiteral("goodreads")), QStringLiteral("https://www.goodreads.com/book/show/53180949-dune"));
  // TODO: create a description field instead of using comments?
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
