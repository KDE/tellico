/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "bibliosharefetchertest.h"

#include "../fetch/bibliosharefetcher.h"
#include "../entry.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( BiblioShareFetcherTest )

BiblioShareFetcherTest::BiblioShareFetcherTest() : AbstractFetcherTest() {
}

void BiblioShareFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  // since we use the importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/biblioshare2tellico.xsl"));
}

void BiblioShareFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0670069035"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BiblioShareFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("The Girl Who Kicked the Hornet's Nest"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Stieg Larsson"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Hardback"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-670-06903-5"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2010"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Penguin Canada"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void BiblioShareFetcherTest::testIsbn13() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("9780670069033"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BiblioShareFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("The Girl Who Kicked the Hornet's Nest"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Stieg Larsson"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Hardback"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-670-06903-5"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2010"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Penguin Canada"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
