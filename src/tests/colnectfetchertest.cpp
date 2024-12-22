/***************************************************************************
    Copyright (C) 2019-2020 Robby Stephenson <robby@periapsis.org>
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

#include "colnectfetchertest.h"

#include "../fetch/colnectfetcher.h"
#include "../entry.h"
#include "../collections/coincollection.h"
#include "../collections/comicbookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( ColnectFetcherTest )

ColnectFetcherTest::ColnectFetcherTest() : AbstractFetcherTest() {
}

void ColnectFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::CoinCollection> registerMe(Tellico::Data::Collection::Coin, "coin");
  Tellico::RegisterCollection<Tellico::Data::ComicBookCollection> registerComic(Tellico::Data::Collection::ComicBook, "comic");

  m_config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("colnect"));
  m_config.writeEntry("Custom Fields", QStringLiteral("obverse,reverse,series,mintage,description"));
}

void ColnectFetcherTest::testSlug() {
  // test the implementation of the Colnect slug derivation
  QFETCH(QString, input);
  QFETCH(QString, slug);

  QCOMPARE(Tellico::Fetch::ColnectFetcher::URLize(input), slug);
}

void ColnectFetcherTest::testSlug_data() {
  QTest::addColumn<QString>("input");
  QTest::addColumn<QString>("slug");

  QTest::newRow("basic") << QStringLiteral("input") << QStringLiteral("input");
  QTest::newRow("Aus1$") << QStringLiteral("1 Dollar (50 Years Moonlanding)") << QStringLiteral("1_Dollar_50_Years_Moonlanding");
}

void ColnectFetcherTest::testRaw() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Coin,
                                       Tellico::Fetch::Raw,
                                       QStringLiteral("147558"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ColnectFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2019"));
  QCOMPARE(entry->field(QStringLiteral("country")), QStringLiteral("Australia"));
  QCOMPARE(entry->field(QStringLiteral("denomination")), QStringLiteral("$1.00"));
  QCOMPARE(entry->field(QStringLiteral("currency")), QStringLiteral("$ - Australian dollar"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("1970~Today - Numismatic Products"));
  QCOMPARE(entry->field(QStringLiteral("mintage")), QStringLiteral("25000"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("obverse")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("obverse")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("reverse")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("reverse")).contains(QLatin1Char('/')));
}

void ColnectFetcherTest::testSacagawea() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Coin,
                                       Tellico::Fetch::Keyword,
                                       QStringLiteral("2007 Sacagawea"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ColnectFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2007"));
  QCOMPARE(entry->field(QStringLiteral("country")), QStringLiteral("United States of America"));
  const auto oneDollar = QLocale::system().toCurrencyString(1.0, QLatin1String("$"));
  QCOMPARE(entry->field(QStringLiteral("denomination")), oneDollar);
  QCOMPARE(entry->field(QStringLiteral("currency")), QStringLiteral("$ - United States dollar"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("B06a - Eisenhower, Anthony & Sacagawea Dollar"));
  QCOMPARE(entry->field(QStringLiteral("mintage")), QStringLiteral("1497251077"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("obverse")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("obverse")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("reverse")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("reverse")).contains(QLatin1Char('/')));
}

void ColnectFetcherTest::testSkylab() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("colnect stamps"));
  cg.writeEntry("Custom Fields", QStringLiteral("series,description,stanley-gibbons,michel,colnect"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Stamp,
                                       Tellico::Fetch::Title,
                                       QStringLiteral("2013 Skylab"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ColnectFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2013"));
  QCOMPARE(entry->field(QStringLiteral("country")), QStringLiteral("Papua New Guinea"));
  QCOMPARE(entry->field(QStringLiteral("stanley-gibbons")), QStringLiteral("PG 1638"));
  QCOMPARE(entry->field(QStringLiteral("michel")), QStringLiteral("PG 1902"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("15th Anniversary of Launch of International Space Station"));
  QCOMPARE(entry->field(QStringLiteral("gummed")), QStringLiteral("PVA (Polyvinyl Alcohol)"));
  const auto oneKina = QLocale::system().toCurrencyString(1.30, QLatin1String("K"));
  QCOMPARE(entry->field(QStringLiteral("denomination")), oneKina);
  QCOMPARE(entry->field(QStringLiteral("currency")), QStringLiteral("K - Papua New Guinean kina"));
  QCOMPARE(entry->field(QStringLiteral("color")), QStringLiteral("Multicolor"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QCOMPARE(entry->field(QStringLiteral("colnect")), QStringLiteral("https://colnect.com/en/stamps/stamp/717470-Skylab_space_station_1"));
  QVERIFY(!entry->field(QStringLiteral("image")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("image")).contains(QLatin1Char('/')));
}

void ColnectFetcherTest::testComic() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("colnect comics"));
  cg.writeEntry("Custom Fields", QStringLiteral("series,colnect"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook,
                                       Tellico::Fetch::Title,
                                       QStringLiteral("Destiny's Hand: Finale"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ColnectFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Destiny's Hand: Finale"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("1993"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("Justice League America (JLA)"));
  QCOMPARE(entry->field(QStringLiteral("writer")), QStringLiteral("Jurgens Dan"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Jurgens Dan; Giordano Dick"));
  QCOMPARE(entry->field(QStringLiteral("issue")), QStringLiteral("75"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("DC Comics"));
  QCOMPARE(entry->field(QStringLiteral("edition")), QStringLiteral("First edition"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Superhero"));
  QCOMPARE(entry->field(QStringLiteral("colnect")), QStringLiteral("https://colnect.com/en/comics/comic/16515-Destinys_Hand_Finale"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void ColnectFetcherTest::testBaseballCard() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("colnect cards"));
  cg.writeEntry("Custom Fields", QStringLiteral("series,colnect"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Card,
                                       Tellico::Fetch::Title,
                                       QStringLiteral("1991 Chipper Jones"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ColnectFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("1991 Upper Deck Chipper Jones"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1991"));
  QCOMPARE(entry->field(QStringLiteral("brand")), QStringLiteral("Upper Deck"));
  QCOMPARE(entry->field(QStringLiteral("team")), QStringLiteral("Atlanta Braves"));
  QCOMPARE(entry->field(QStringLiteral("number")), QStringLiteral("55"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("Base Set"));
  QCOMPARE(entry->field(QStringLiteral("type")), QStringLiteral("Major League Baseball"));
  QCOMPARE(entry->field(QStringLiteral("colnect")), QStringLiteral("https://colnect.com/en/sports_cards/sports_card/67064-55_Chipper_Jones_1991"));
  QVERIFY(!entry->field(QStringLiteral("front")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("front")).contains(QLatin1Char('/')));
}

void ColnectFetcherTest::testGoldeneye() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("colnect games"));
  cg.writeEntry("Custom Fields", QStringLiteral("pegi"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game,
                                       Tellico::Fetch::Title,
                                       QStringLiteral("Goldeneye 007"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ColnectFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 2);

  QVERIFY(results.size() > 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("GoldenEye 007"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1997"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Nintendo"));
  QCOMPARE(entry->field(QStringLiteral("platform")), QStringLiteral("Nintendo 64"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("FPS"));
  QCOMPARE(entry->field(QStringLiteral("pegi")), QStringLiteral("PEGI 16"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));

  // test against the Japanese version, which is result #2
  entry = results.at(1);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("GoldenEye 007"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1997"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Nintendo"));
  QCOMPARE(entry->field(QStringLiteral("platform")), QStringLiteral("Nintendo 64"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Shooter"));
  QCOMPARE(entry->field(QStringLiteral("certification")), QStringLiteral("Teen"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
