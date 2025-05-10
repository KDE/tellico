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

#include "upcitemdbfetchertest.h"

#include "../fetch/upcitemdbfetcher.h"
#include "../collectionfactory.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/boardgamecollection.h"
#include "../collections/musiccollection.h"
#include "../collections/gamecollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( UPCItemDbFetcherTest )

UPCItemDbFetcherTest::UPCItemDbFetcherTest() : AbstractFetcherTest() {
}

void UPCItemDbFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::RegisterCollection<Tellico::Data::BoardGameCollection> registerBoardGame(Tellico::Data::Collection::BoardGame, "boardgame");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  Tellico::RegisterCollection<Tellico::Data::GameCollection> registerGame(Tellico::Data::Collection::Game, "game");
  Tellico::ImageFactory::init();
}

void UPCItemDbFetcherTest::testFightClub() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("upcitemdb"));
  cg.writeEntry("Custom Fields", QStringLiteral("barcode"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::UPC,
                                       QStringLiteral("024543617907"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::UPCItemDbFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Fight Club"));
//  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1999"));
  QCOMPARE(entry->field(QStringLiteral("barcode")), QStringLiteral("024543617907"));
  QCOMPARE(entry->field(QStringLiteral("medium")), QStringLiteral("Blu-ray"));
  QCOMPARE(entry->field(QStringLiteral("studio")), QStringLiteral("20th Century Studios"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void UPCItemDbFetcherTest::testCatan() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::BoardGame, Tellico::Fetch::UPC,
                                       QStringLiteral("029877030712"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::UPCItemDbFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Settlers of Catan Board Game"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Catan Studio"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
}

void UPCItemDbFetcherTest::test1632() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0671319728"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::UPCItemDbFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("1632"));
//  QEXPECT_FAIL("", "Author data is in publisher field", Continue);
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Eric Flint"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("9780671319724"));
//  QEXPECT_FAIL("", "Author data is in publisher field", Continue);
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Baen"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
//  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
}

void UPCItemDbFetcherTest::testBurningEdge() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::UPC,
                                       QStringLiteral("829619128628"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::UPCItemDbFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("The Burning Edge of Dawn"));
  QEXPECT_FAIL("", "Artist data is in brand field", Continue);
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Andrew Peterson"));
  QEXPECT_FAIL("", "Artist data is in brand field", Continue);
  QCOMPARE(entry->field(QStringLiteral("label")), QStringLiteral("UMGD"));
  QCOMPARE(entry->field(QStringLiteral("medium")), QStringLiteral("Compact Disc"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void UPCItemDbFetcherTest::testGTA4() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::UPC,
                                       QStringLiteral("710425392320"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::UPCItemDbFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Grand Theft Auto IV"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Take Two Interactive"));
  QCOMPARE(entry->field(QStringLiteral("platform")), QStringLiteral("Xbox 360"));
  // cover image is timing out for some reason
//  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
//  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
}
