/***************************************************************************
    Copyright (C) 2009-2011 Robby Stephenson <robby@periapsis.org>
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

#include "imdbfetchertest.h"

#include "../fetch/imdbfetcher.h"
#include "../entry.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"
#include "../fetch/fetcherjob.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( ImdbFetcherTest )

ImdbFetcherTest::ImdbFetcherTest() : AbstractFetcherTest() {
}

void ImdbFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
}

void ImdbFetcherTest::testSnowyRiver() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QLatin1String("IMDB");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "The Man From Snowy River");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QLatin1String("The Man from Snowy River"));
  QCOMPARE(entry->field("year"), QLatin1String("1982"));
  QCOMPARE(set(entry, "genre"), set("Adventure; Drama; Family; Romance; Western"));
  QCOMPARE(entry->field("nationality"), QLatin1String("Australia"));
  QCOMPARE(set(entry, "studio"), set("Cambridge Productions; Michael Edgley International; Snowy River Investment Pty. Ltd."));
  QCOMPARE(entry->field("running-time"), QLatin1String("102"));
  QCOMPARE(entry->field("audio-track"), QLatin1String("Dolby"));
  QCOMPARE(entry->field("aspect-ratio"), QLatin1String("2.35 : 1"));
  QCOMPARE(entry->field("color"), QLatin1String("Color"));
  QCOMPARE(entry->field("language"), QLatin1String("English"));
  QCOMPARE(entry->field("certification"), QLatin1String("PG (USA)"));
  QCOMPARE(entry->field("director"), QLatin1String("George Miller"));
  QCOMPARE(entry->field("producer"), QLatin1String("Geoff Burrowes"));
  QCOMPARE(set(entry, "writer"), set("Cul Cullen; A.B. 'Banjo' Paterson;John Dixon"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QLatin1String("Tom Burlinson::Jim Craig"));
  QCOMPARE(entry->field("imdb"), QLatin1String("http://www.imdb.com/title/tt0084296/"));
  QVERIFY(!entry->field("imdb-rating").isEmpty());
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).contains(QLatin1Char('/')));
  QStringList altTitleList = Tellico::FieldFormat::splitTable(entry->field("alttitle"));
  QVERIFY(altTitleList.contains(QLatin1String("Herencia de un valiente")));
  QVERIFY(!entry->field("alttitle").contains(QLatin1String("See more")));
}

void ImdbFetcherTest::testAsterix() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QLatin1String("IMDB");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QString::fromUtf8("Astérix aux jeux olympiques"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  // title is returned in english
  QCOMPARE(entry->field("title"), QLatin1String("Asterix at the Olympic Games"));
  QCOMPARE(entry->field("origtitle"), QString::fromUtf8("Astérix aux jeux olympiques"));
  QCOMPARE(set(entry, "director"), set(QString::fromUtf8("Thomas Langmann; Frédéric Forestier")));
  QCOMPARE(set(entry, "writer"), set(QString::fromUtf8("Franck Magnier; René Goscinny; Olivier Dazat; Alexandre Charlot; Thomas Langmann; Albert Uderzo")));
  QStringList altTitleList = Tellico::FieldFormat::splitTable(entry->field("alttitle"));
  QVERIFY(altTitleList.contains(QString::fromUtf8("Asterix at the Olympic Games")));
  QVERIFY(altTitleList.contains(QString::fromUtf8("Astérix en los Juegos Olímpicos")));
  QVERIFY(altTitleList.contains(QLatin1String("Asterix alle olimpiadi")));
}

// https://bugs.kde.org/show_bug.cgi?id=249096
void ImdbFetcherTest::testBodyDouble() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "Body Double");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QLatin1String("Body Double"));
  QCOMPARE(entry->field("director"), QLatin1String("Brian De Palma"));
  QCOMPARE(set(entry, "writer"), set("Brian De Palma; Robert J. Avrech"));
  QCOMPARE(entry->field("producer"), QLatin1String("Brian De Palma"));
}

// https://bugs.kde.org/show_bug.cgi?id=249096
void ImdbFetcherTest::testMary() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "There's Something About Mary");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(set(entry, "director"), set("Peter Farrelly; Bobby Farrelly"));
  QCOMPARE(set(entry, "writer"), set("John J. Strauss; Ed Decter; Peter Farrelly; Bobby Farrelly"));
}

// https://bugs.kde.org/show_bug.cgi?id=262036
void ImdbFetcherTest::testOkunen() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QLatin1String("IMDB Okunen");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "46-okunen no koi");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  // if the settings included origtitle, then the title would be "Big Bang Love, Juvenile A"
  QCOMPARE(entry->field("title"), QLatin1String("46-okunen no koi"));
  QCOMPARE(entry->field("origtitle"), QLatin1String(""));
  QCOMPARE(entry->field("year"), QLatin1String("2006"));
  QCOMPARE(entry->field("genre"), QLatin1String("Drama; Fantasy"));
  QCOMPARE(entry->field("director"), QLatin1String("Takashi Miike"));
  QCOMPARE(set(entry, "writer"), set("Ikki Kajiwara; Hisao Maki; Masa Nakamura"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).contains(QLatin1Char('/')));
  QStringList altTitleList = Tellico::FieldFormat::splitTable(entry->field("alttitle"));
  QVERIFY(altTitleList.contains(QLatin1String("Big Bang Love, Juvenile A")));
}

// https://bugs.kde.org/show_bug.cgi?id=314113
void ImdbFetcherTest::testFetchResultEncoding() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QString::fromUtf8("jôbafuku onna harakiri"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  if(!hasNetwork()) {
    QSKIP("This test requires network access", SkipSingle);
    return;
  }

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->title(), QString::fromUtf8("'Shitsurakuen': jôbafuku onna harakiri"));
}

// https://bugs.kde.org/show_bug.cgi?id=336765
void ImdbFetcherTest::testBabel() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "Babel");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QLatin1String("Babel"));
  QCOMPARE(entry->field("year"), QLatin1String("2006"));
  QCOMPARE(entry->field("director"), QString::fromUtf8("Alejandro G. Iñárritu"));
  QCOMPARE(set(entry, "writer"), set(QString::fromUtf8("Alejandro G. Iñárritu; Guillermo Arriaga")));
  // I can't figure out why this test has to use fromLocal8Bit instead of fromUtf8. The actual Tellico output is the same.
  QCOMPARE(set(entry, "producer"), set(QString::fromLocal8Bit("Steve Golin; Alejandro G. Iñárritu; Jon Kilik; Ann Ruark; Corinne Golden Weber")));
}
