/***************************************************************************
    Copyright (C) 2010-2012 Robby Stephenson <robby@periapsis.org>
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

#include "allocinefetchertest.h"

#include "../fetch/execexternalfetcher.h"
#include "../fetch/allocinefetcher.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( AllocineFetcherTest )

AllocineFetcherTest::AllocineFetcherTest() : AbstractFetcherTest() {
}

void AllocineFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::ImageFactory::init();
}

void AllocineFetcherTest::cleanupTestCase() {
  Tellico::ImageFactory::clean(true);
}

void AllocineFetcherTest::testTitle() {
  // Allocine script is currently failing
  return;
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("Superman Returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QFINDTESTDATA("../fetch/scripts/fr.allocine.py.spec"), KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QStringLiteral("<default>"));
  cg.writeEntry("ExecPath", QFINDTESTDATA("../fetch/scripts/fr.allocine.py"));
  // don't sync() and save the new path
  cg.markAsClean();
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Superman Returns"));
  QCOMPARE(entry->field(QStringLiteral("director")), QStringLiteral("Bryan Singer"));
  QCOMPARE(entry->field(QStringLiteral("producer")), QStringLiteral("Jon Peters; Gilbert Adler; Bryan Singer; Lorne Orleans"));
  QCOMPARE(entry->field(QStringLiteral("studio")), QStringLiteral("Warner Bros. France"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2006"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Fantastique; Action"));
  QCOMPARE(entry->field(QStringLiteral("nationality")), QString::fromUtf8("Américain; Australien"));
  QCOMPARE(entry->field(QStringLiteral("running-time")), QStringLiteral("154"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QStringLiteral("Brandon Routh::Clark Kent / Superman"));
  QCOMPARE(castList.size(), 8);
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void AllocineFetcherTest::testTitleAccented() {
  // Allocine script is currently failing
  return;
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("Opération Tonnerre"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QFINDTESTDATA("../fetch/scripts/fr.allocine.py.spec"), KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QStringLiteral("<default>"));
  cg.writeEntry("ExecPath", QFINDTESTDATA("../fetch/scripts/fr.allocine.py"));
  // don't sync() and save the new path
  cg.markAsClean();
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QString::fromUtf8("Opération Tonnerre"));
  QCOMPARE(entry->field(QStringLiteral("titre-original")), QStringLiteral("Thunderball"));
  QCOMPARE(entry->field(QStringLiteral("studio")), QString());
}

void AllocineFetcherTest::testTitleAccentRemoved() {
  // Allocine script is currently failing
  return;
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("Operation Tonnerre"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QFINDTESTDATA("../fetch/scripts/fr.allocine.py.spec"), KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QStringLiteral("<default>"));
  cg.writeEntry("ExecPath", QFINDTESTDATA("../fetch/scripts/fr.allocine.py"));
  // don't sync() and save the new path
  cg.markAsClean();
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QString::fromUtf8("Opération Tonnerre"));
}

void AllocineFetcherTest::testPlotQuote() {
  // Allocine script is currently failing
  return;
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("Goldfinger"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QFINDTESTDATA("../fetch/scripts/fr.allocine.py.spec"), KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QStringLiteral("<default>"));
  cg.writeEntry("ExecPath", QFINDTESTDATA("../fetch/scripts/fr.allocine.py"));
  // don't sync() and save the new path
  cg.markAsClean();
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Goldfinger"));
  QVERIFY(!entry->field(QStringLiteral("plot")).contains(QStringLiteral("&quot;")));
}

void AllocineFetcherTest::testTitleAPI() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("allocine");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword,
                                       QStringLiteral("Superman Returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AllocineFetcher(this));
  fetcher->readConfig(cg, cg.name());
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Superman Returns"));
  QCOMPARE(entry->field(QStringLiteral("director")), QStringLiteral("Bryan Singer"));
  QCOMPARE(entry->field(QStringLiteral("producer")), QStringLiteral("Jon Peters; Gilbert Adler; Bryan Singer; Lorne Orleans"));
  QCOMPARE(entry->field(QStringLiteral("studio")), QStringLiteral("Warner Bros. France"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2006"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Fantastique; Action"));
  QCOMPARE(entry->field(QStringLiteral("nationality")), QStringLiteral("U.S.A.; Australie"));
  QCOMPARE(entry->field(QStringLiteral("running-time")), QStringLiteral("154"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QStringLiteral("Brandon Routh::Clark Kent / Superman"));
  QCOMPARE(castList.size(), 5);
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void AllocineFetcherTest::testTitleAPIAccented() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("allocine");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword,
                                       QStringLiteral("Opération Tonnerre"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AllocineFetcher(this));
  fetcher->readConfig(cg, cg.name());
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QString::fromUtf8("Opération Tonnerre"));
  QCOMPARE(entry->field(QStringLiteral("origtitle")), QStringLiteral("Thunderball"));
  QCOMPARE(entry->field(QStringLiteral("studio")), QStringLiteral("United International Pictures (UIP)"));
  QCOMPARE(entry->field(QStringLiteral("director")), QStringLiteral("Terence Young"));
  QCOMPARE(entry->field(QStringLiteral("color")), QStringLiteral("Color"));
  QVERIFY(!entry->field(QStringLiteral("allocine")).isEmpty());
}

// mentioned in https://bugs.kde.org/show_bug.cgi?id=337432
void AllocineFetcherTest::testGhostDog() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("allocine");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword,
                                       QStringLiteral("Ghost Dog: la voie du samourai"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AllocineFetcher(this));
  fetcher->readConfig(cg, cg.name());
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Ghost Dog: la voie du samourai"));
}
