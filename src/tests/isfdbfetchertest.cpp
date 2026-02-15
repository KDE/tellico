/***************************************************************************
    Copyright (C) 2025 Robby Stephenson <robby@periapsis.org>
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

#include "isfdbfetchertest.h"

#include "../fetch/isfdbfetcher.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( ISFDBFetcherTest )

ISFDBFetcherTest::ISFDBFetcherTest() : AbstractFetcherTest() {
}

void ISFDBFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/isfdb2tellico.xsl"));
  Tellico::ImageFactory::init();

  m_config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("isfdb"));
  m_config.writeEntry("Custom Fields", QStringLiteral("isfdb"));
}

void ISFDBFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("1881475530"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISFDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("The Winchester Horror"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("William F. Nolan"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("1-881475-53-0"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("1998"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Cemetery Dance Publications"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Hardback"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("111"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("Cemetery Dance Novella Series"));
  QCOMPARE(entry->field(QStringLiteral("series_num")), QStringLiteral("6"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void ISFDBFetcherTest::testIsbn13() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("978-0-7434-3538-3"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISFDBFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("March Upcountry"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("John Ringo; David Weber"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-7434-3538-9")); // returns in isbn10 format
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2002"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Baen Books"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("586"));
  QCOMPARE(entry->field(QStringLiteral("isfdb")), QStringLiteral("https://www.isfdb.org/cgi-bin/pl.cgi?21426"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

// publisher text is split manually
void ISFDBFetcherTest::testPublisher() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0439176832"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISFDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Castle"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2000"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Lucas Books; Scholastic"));
}

void ISFDBFetcherTest::testLccn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::LCCN,
                                       QStringLiteral("53001807"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISFDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Prize Science Fiction"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Donald A. Wollheim"));
  QCOMPARE(entry->field(QStringLiteral("lccn")), QStringLiteral("53001807"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("1953"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("The McBride Company"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("230"));
}
