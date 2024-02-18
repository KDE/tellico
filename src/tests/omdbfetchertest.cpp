/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include "omdbfetchertest.h"

#include "../fetch/omdbfetcher.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( OMDBFetcherTest )

OMDBFetcherTest::OMDBFetcherTest() : AbstractFetcherTest() {
}

void OMDBFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();

  QString configFile = QFINDTESTDATA("tellicotest_private.config");
  if(!configFile.isEmpty()) {
    m_config = KSharedConfig::openConfig(configFile, KConfig::SimpleConfig)->group(QStringLiteral("OMDB"));
  }
}

void OMDBFetcherTest::testTitle() {
  // all the private API test config is in tellicotest_private.config right now
  if(!m_config.isValid()) {
    QSKIP("This test requires a config file.", SkipAll);
  }

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("superman returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OMDBFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Superman Returns"));
  QCOMPARE(entry->field(QStringLiteral("certification")), QStringLiteral("PG-13 (USA)"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2006"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Action; Adventure; Sci-Fi"));
  QCOMPARE(entry->field(QStringLiteral("director")), QStringLiteral("Bryan Singer"));
  QCOMPARE(set(entry, "writer"),
           set("Bryan Singer; Dan Harris; Michael Dougherty"));
//  QCOMPARE(entry->field(QStringLiteral("producer")), QStringLiteral("Bryan Singer; Jon Peters; Gilbert Adler"));
  QCOMPARE(entry->field(QStringLiteral("running-time")), QStringLiteral("154"));
  QCOMPARE(entry->field(QStringLiteral("nationality")), QStringLiteral("United States; Australia"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QStringLiteral("Brandon Routh"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
  QCOMPARE(entry->field(QStringLiteral("imdb")), QStringLiteral("https://www.imdb.com/title/tt0348150/"));
}

// see https://bugs.kde.org/show_bug.cgi?id=336765
void OMDBFetcherTest::testBabel() {
  if(!m_config.isValid()) {
    QSKIP("This test requires a config file.", SkipAll);
  }

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("babel"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OMDBFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("title"), QStringLiteral("Babel"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("director"), QString::fromUtf8("Alejandro G. Iñárritu"));
}

void OMDBFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::VideoCollection(true));
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("imdb"),
                                                         QStringLiteral("IMDB"),
                                                         Tellico::Data::Field::URL));
  QCOMPARE(coll->addField(field), true);
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);
  entry->setField(QStringLiteral("title"), QStringLiteral("The Man From Snowy River"));
  entry->setField(QStringLiteral("year"), QStringLiteral("1982"));

  Tellico::Fetch::OMDBFetcher fetcher(this);
  auto request = fetcher.updateRequest(entry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QVERIFY(request.value().contains(QStringLiteral("y=1982")));

  entry->setField(QStringLiteral("imdb"), QStringLiteral("https://www.imdb.com/title/tt0084296/?ref_=nv_sr_srsg_0"));
  request = fetcher.updateRequest(entry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QVERIFY(request.value().contains(QStringLiteral("i=tt0084296")));
}
