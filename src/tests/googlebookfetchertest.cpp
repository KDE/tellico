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

#include "googlebookfetchertest.h"

#include "../fetch/googlebookfetcher.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( GoogleBookFetcherTest )

GoogleBookFetcherTest::GoogleBookFetcherTest() : AbstractFetcherTest()
    , m_config(QFINDTESTDATA("/tellicotest.config"), KConfig::SimpleConfig) {
  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest.config"));
}

void GoogleBookFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void GoogleBookFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("Practical Rdf"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoogleBookFetcher(this));
  if(m_hasConfigFile) {
    KConfigGroup cg(&m_config, QStringLiteral("GoogleBookTest"));
    fetcher->readConfig(cg, cg.name());
  }

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  compareEntry(results.at(0));
}

void GoogleBookFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0-596-55051-0"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoogleBookFetcher(this));
  if(m_hasConfigFile) {
    KConfigGroup cg(&m_config, QStringLiteral("GoogleBookTest"));
    fetcher->readConfig(cg, cg.name());
  }

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  compareEntry(results.at(0));
}

void GoogleBookFetcherTest::testAuthor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Person,
                                       QStringLiteral("Shelley Powers"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoogleBookFetcher(this));
  if(m_hasConfigFile) {
    KConfigGroup cg(&m_config, QStringLiteral("GoogleBookTest"));
    fetcher->readConfig(cg, cg.name());
  }

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr testEntry, results) {
    if(testEntry->title() == QStringLiteral("Practical RDF")) {
      entry = testEntry;
      break;
    }
  }
  QVERIFY(entry);
  compareEntry(entry);
}

void GoogleBookFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Keyword,
                                       QStringLiteral("Practical Rdf"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoogleBookFetcher(this));
  if(m_hasConfigFile) {
    KConfigGroup cg(&m_config, QStringLiteral("GoogleBookTest"));
    fetcher->readConfig(cg, cg.name());
  }

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  compareEntry(results.at(0));
}

void GoogleBookFetcherTest::compareEntry(Tellico::Data::EntryPtr entry) {
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Practical RDF"));
  if(entry->field(QStringLiteral("isbn")) == QStringLiteral("0-596-55051-0")) {
    QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-596-55051-0"));
    QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("352"));
  } else {
    QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-596-51561-8"));
    QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("336"));
  }
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Shelley Powers"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("O'Reilly Media, Inc."));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2003"));
  QVERIFY(entry->field(QStringLiteral("keyword")).contains(QStringLiteral("Computers")));
  QVERIFY(entry->field(QStringLiteral("keyword")).contains(QStringLiteral("XML")));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
}
