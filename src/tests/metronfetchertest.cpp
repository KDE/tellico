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

#include "metronfetchertest.h"

#include "../fetch/metronfetcher.h"
#include "../collections/comicbookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( MetronFetcherTest )

MetronFetcherTest::MetronFetcherTest() : AbstractFetcherTest() {
}

void MetronFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::ComicBookCollection> registerCB(Tellico::Data::Collection::ComicBook, "comic");
  Tellico::ImageFactory::init();

  auto configFile = QFINDTESTDATA("tellicotest_private.config");
  if(QFile::exists(configFile)) {
    m_config = KSharedConfig::openConfig(configFile, KConfig::SimpleConfig)->group(QStringLiteral("Metron"));
  }
}

void MetronFetcherTest::testKeyword() {
  if(!m_config.isValid()) {
    QSKIP("This test requires a config file with Metron settings.", SkipAll);
  }

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook,
                                       Tellico::Fetch::Keyword,
                                       QStringLiteral("The Avengers #1"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MetronFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("The Coming of the Avengers!"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("1963"));
  QCOMPARE(entry->field(QStringLiteral("issue")), QStringLiteral("1"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Super-Hero"));
  QCOMPARE(entry->field(QStringLiteral("writer")), QStringLiteral("Stan Lee"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Dick Ayers; Jack Kirby"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("The Avengers"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Marvel"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("36"));
  QCOMPARE(entry->field(QStringLiteral("metron")), QStringLiteral("https://metron.cloud/issue/the-avengers-1963-1/"));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void MetronFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::ComicBookCollection(true));
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);
  entry->setField(QStringLiteral("series"), QStringLiteral("The Avengers"));
  entry->setField(QStringLiteral("issue"), QStringLiteral("1"));

  Tellico::Fetch::MetronFetcher fetcher(this);
  auto request = fetcher.updateRequest(entry);
  QCOMPARE(request.key(), Tellico::Fetch::Keyword);
  QCOMPARE(request.value(), QStringLiteral("The Avengers #1"));
}
