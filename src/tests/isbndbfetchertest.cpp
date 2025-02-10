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

ISBNdbFetcherTest::ISBNdbFetcherTest() : AbstractFetcherTest() {
}

void ISBNdbFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest_private.config"));
  if(m_hasConfigFile) {
    m_config = KSharedConfig::openConfig(QFINDTESTDATA("tellicotest_private.config"), KConfig::SimpleConfig);
  }
}

void ISBNdbFetcherTest::testIsbnLocal() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("isbndb"));
  cg.writeEntry("Custom Fields", QStringLiteral("dewey"));

  QUrl testUrl1 = QUrl::fromLocalFile(QFINDTESTDATA("data/isbndb_isbn.json"));
  auto f = new Tellico::Fetch::ISBNdbFetcher(this);
  f->setTestUrl1(testUrl1);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0620062126"));
  Tellico::Fetch::Fetcher::Ptr fetcher(f);
  fetcher->readConfig(cg);

  Tellico::Fetch::MessageLogger* logger = new Tellico::Fetch::MessageLogger;
  fetcher->setMessageHandler(logger);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);
  QVERIFY(logger->errorList.isEmpty());

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Killie's Africa"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Herd, Norman"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0620062126"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("1982"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Blue Crane Books"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("eng"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("200"));
  QCOMPARE(entry->field(QStringLiteral("dewey")), QStringLiteral("968.05/092/4"));
  QCOMPARE(entry->field(QStringLiteral("plot")),
           QStringLiteral("Includes bibliographical references and index."));
  QCOMPARE(entry->field(QStringLiteral("genre")),
           QStringLiteral("campbell killie 1881 1965; historians south africa biography; "
                          "antiques south africa biography; book collectors south africa biography"));
}

void ISBNdbFetcherTest::testIsbn() {
  QString groupName = QStringLiteral("ISBNdb");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with ISBNdb settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0789312239"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  Tellico::Fetch::MessageLogger* logger = new Tellico::Fetch::MessageLogger;
  fetcher->setMessageHandler(logger);
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);
  QVERIFY(logger->errorList.isEmpty());

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

void ISBNdbFetcherTest::testIsbn13() {
  QString groupName = QStringLiteral("ISBNdb");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with ISBNdb settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("9780789312235"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  fetcher->readConfig(cg);

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
  const bool batchIsbn = true;
  QString groupName = QStringLiteral("ISBNdb");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with ISBNdb settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);
  cg.writeEntry("Batch ISBN", batchIsbn);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0789312239; 0393339912"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 2);

  // can't be sure which entry matches which ISBN

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  const bool veniceIsFirst = entry->field(QStringLiteral("isbn")) == QStringLiteral("0789312239");
  if(!veniceIsFirst) {
    entry = results.at(1);
    QVERIFY(entry);
  }

  QCOMPARE(entry->field(QStringLiteral("title")).toLower(), QStringLiteral("this is venice"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Sasek, M. (miroslav)"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0789312239"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2005"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Universe"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Hardback"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Description And Travel"));
  // no cover in batch mode
  if(!batchIsbn) {
    QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
    QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  }

  entry = results.at(veniceIsFirst ? 1 : 0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Packing For Mars: The Curious Science Of Life In The Void"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Mary Roach"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0393339912"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2011"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("W. W. Norton & Company"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  if(!batchIsbn) {
    QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
    QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  }
}

void ISBNdbFetcherTest::testTitle() {
  QString groupName = QStringLiteral("ISBNdb");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with ISBNdb settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("PACKING FOR MARS The Curious Science of Life in the Void"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr testEntry, results) {
    if(testEntry->field(QStringLiteral("isbn")).remove('-') == QStringLiteral("0393339912")) {
      entry = testEntry;
      break;
    }
  }
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Packing For Mars: The Curious Science Of Life In The Void"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Mary Roach"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0393339912"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2011"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("W. W. Norton & Company"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void ISBNdbFetcherTest::testAuthor() {
  QString groupName = QStringLiteral("ISBNdb");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with ISBNdb settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Person,
                                       QStringLiteral("Joshua Foer"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr testEntry, results) {
    if(testEntry->field(QStringLiteral("isbn")).remove('-') == QStringLiteral("0143120530")) {
      entry = testEntry;
      break;
    }
  }
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Moonwalking With Einstein: The Art And Science Of Remembering Everything"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Joshua Foer"));
  QCOMPARE(entry->field(QStringLiteral("isbn")).remove('-'), QStringLiteral("0143120530"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2012"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Penguin Books"));
  QEXPECT_FAIL("", "ISBNdb author search does not seem to include binding", Continue);
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
}
