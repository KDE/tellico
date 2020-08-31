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

#include "isbndbfetchertest.h"

#include "../fetch/isbndbfetcher.h"
#include "../fetch/messagelogger.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( ISBNdbFetcherTest )

ISBNdbFetcherTest::ISBNdbFetcherTest() : AbstractFetcherTest()
    , m_config(QFINDTESTDATA("tellicotest_private.config"), KConfig::SimpleConfig) {
  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest_private.config"));
}

void ISBNdbFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void ISBNdbFetcherTest::testIsbn() {
  QString groupName = QStringLiteral("ISBNdb");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with ISBNdb settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0789312239"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  Tellico::Fetch::MessageLogger* logger = new Tellico::Fetch::MessageLogger;
  fetcher->setMessageHandler(logger);
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);
  QVERIFY(logger->errorList.isEmpty());

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")).toLower(), QStringLiteral("this is venice"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("M. Sasek"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0789312239"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2008"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Universe Publishing Inc.,U.S."));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Hardback"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Children's Non-Fiction; People & Places"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void ISBNdbFetcherTest::testIsbn13() {
  QString groupName = QStringLiteral("ISBNdb");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with ISBNdb settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("9780789312235"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")).toLower(), QStringLiteral("this is venice"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Sasek, M. (miroslav)"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0789312239"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2005"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Universe"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Hardback"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Description And Travel"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void ISBNdbFetcherTest::testMultipleIsbn() {
  QString groupName = QStringLiteral("ISBNdb");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with ISBNdb settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0789312239; 9780596000486"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QEXPECT_FAIL("", "ISBNdb fetcher does not yet support searching for multiple ISBNs", Continue);
  QCOMPARE(results.size(), 2);
}

void ISBNdbFetcherTest::testTitle() {
  QString groupName = QStringLiteral("ISBNdb");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with ISBNdb settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("PACKING FOR MARS The Curious Science of Life in the Void"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr testEntry, results) {
    if(testEntry->field(QStringLiteral("isbn")) == QStringLiteral("0393339912")) {
      entry = testEntry;
      break;
    }
  }
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Packing for Mars: The Curious Science of Life in the Void"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Mary Roach"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0393339912"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2011"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("W. W. Norton & Company"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QCOMPARE(entry->field(QStringLiteral("edition")), QStringLiteral("0"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void ISBNdbFetcherTest::testAuthor() {
  QString groupName = QStringLiteral("ISBNdb");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with ISBNdb settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Person,
                                       QStringLiteral("Joshua Foer"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry= results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Moonwalking with Einstein"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Joshua Foer"));
  QCOMPARE(entry->field(QStringLiteral("isbn")).remove('-'), QStringLiteral("0143120530"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2012"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Penguin (Non-Classics)"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
}
