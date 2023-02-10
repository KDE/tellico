/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "itunesfetchertest.h"

#include "../fetch/itunesfetcher.h"
#include "../collections/musiccollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( ItunesFetcherTest )

ItunesFetcherTest::ItunesFetcherTest() : AbstractFetcherTest() {
}

void ItunesFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void ItunesFetcherTest::testBurningEdge() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("itunes"));
  cg.writeEntry("Custom Fields", QStringLiteral("itunes"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QStringLiteral("Burning Edge Of Dawn"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ItunesFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("The Burning Edge of Dawn"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Andrew Peterson"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("CCM"));
  QCOMPARE(entry->field(QStringLiteral("itunes")), QStringLiteral("https://music.apple.com/us/album/the-burning-edge-of-dawn/1560397363?uo=4"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));

  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.count(), 10);
  QCOMPARE(trackList.at(0), QStringLiteral("The Dark Before the Dawn::Andrew Peterson::4:09"));
}

void ItunesFetcherTest::testUpc() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::UPC,
                                       QStringLiteral("829619128628"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ItunesFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("The Burning Edge of Dawn"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Andrew Peterson"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));

  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.count(), 10);
  QCOMPARE(trackList.at(0), QStringLiteral("The Dark Before the Dawn::Andrew Peterson::4:09"));
}

void ItunesFetcherTest::testTopGun() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword,
                                       QStringLiteral("Top Gun"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ItunesFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Top Gun"));
  QCOMPARE(entry->field(QStringLiteral("director")), QStringLiteral("Tony Scott"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1986"));
  QCOMPARE(entry->field(QStringLiteral("nationality")), QStringLiteral("USA"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Action & Adventure"));
  QCOMPARE(entry->field(QStringLiteral("certification")), QStringLiteral("PG (USA)"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(entry->field(QStringLiteral("plot")).startsWith(QLatin1String("A hip, heart-pounding combination of action")));
}
