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

#include "hardcoverfetchertest.h"

#include "../fetch/hardcoverfetcher.h"
#include "../collections/bookcollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( HardcoverFetcherTest )

HardcoverFetcherTest::HardcoverFetcherTest() : AbstractFetcherTest() {
}

void HardcoverFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();

  auto configFile = QFINDTESTDATA("tellicotest_private.config");
  if(QFile::exists(configFile)) {
    m_config = KSharedConfig::openConfig(configFile, KConfig::SimpleConfig)->group(QStringLiteral("Hardcover"));
  }
}

void HardcoverFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book,
                                       Tellico::Fetch::ISBN,
                                       QStringLiteral("044117-27-17;0765377063"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::HardcoverFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));
  QVERIFY(fetcher->canSearchMultiple());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);
  QCOMPARE(results.size(), 2);

  Tellico::Data::EntryPtr entry = results.at(0);
  Tellico::Data::EntryPtr entry2 = results.at(1);
  QVERIFY(entry);
  if(entry->field(QStringLiteral("isbn")) != QLatin1StringView("0-441-17271-7")) {
    entry2 = entry;
    entry = results.at(1);
    QVERIFY(entry);
  }
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Dune"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Frank Herbert"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-441-17271-7"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Penguin"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("1965"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Science Fiction"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("704"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("English"));
  QCOMPARE(entry->field(QStringLiteral("hardcover")), QStringLiteral("https://hardcover.app/edition/id/30426415"));
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));

  // check series and translator
  QCOMPARE(entry2->field(QStringLiteral("title")), QStringLiteral("The Three-Body Problem"));
  QCOMPARE(entry2->field(QStringLiteral("author")), QStringLiteral("Cixin Liu"));
  QCOMPARE(entry2->field(QStringLiteral("translator")), QStringLiteral("Ken Liu"));
  QCOMPARE(entry2->field(QStringLiteral("series")), QStringLiteral("Remembrance of Earth's Past"));
  QCOMPARE(entry2->field(QStringLiteral("series_num")), QStringLiteral("1"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2006"));
}

void HardcoverFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book,
                                       Tellico::Fetch::Title,
                                       QStringLiteral("Packing For Mars"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::HardcoverFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));
  QVERIFY(fetcher->canSearchMultiple());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 5);
  QCOMPARE(results.size(), 5);

  Tellico::Data::EntryPtr entry;
  for(auto tmpEntry : results) {
    if(tmpEntry->field(QStringLiteral("isbn")) == QLatin1String("0-393-07910-4")) {
      entry = tmpEntry;
      break;
    }
  }
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Packing for Mars: The Curious Science of Life in the Void"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Mary Roach"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-393-07910-4"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2011"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("W. W. Norton & Company"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("E-Book"));
  QVERIFY(entry->field(QStringLiteral("genre")).contains(QStringLiteral("Science")));
  QVERIFY(entry->field(QStringLiteral("genre")).contains(QStringLiteral("Space")));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("335"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("English"));
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
