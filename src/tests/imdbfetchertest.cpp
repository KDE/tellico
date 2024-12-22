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

#include "imdbfetchertest.h"

#include "../fetch/imdbfetcher.h"
#include "../entry.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../images/imageinfo.h"
#include "../fieldformat.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( ImdbFetcherTest )

ImdbFetcherTest::ImdbFetcherTest() : AbstractFetcherTest() {
}

void ImdbFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");

  m_config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("IMDB"));
  m_config.writeEntry("Custom Fields", QStringLiteral("alttitle,imdb,imdb-rating,origtitle"));
}

void ImdbFetcherTest::init() {
  // reset to system locale every time
  QLocale::setDefault(QLocale::system());
}

void ImdbFetcherTest::testSnowyRiver() {
  m_config.writeEntry("Image Size", 1); // small
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("The Man From Snowy River"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("The Man from Snowy River"));
  QCOMPARE(entry->field("year"), QStringLiteral("1982"));
  QCOMPARE(set(entry, "genre"), set("Adventure; Drama; Romance; Western"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("Australia"));
  QCOMPARE(set(entry, "studio"), set("Cambridge Productions; Michael Edgley International; Snowy River Investment Pty. Ltd."));
  QCOMPARE(entry->field("running-time"), QStringLiteral("102"));
  QCOMPARE(entry->field("audio-track"), QStringLiteral("Dolby"));
  QCOMPARE(entry->field("aspect-ratio"), QStringLiteral("2.35 : 1"));
  QCOMPARE(entry->field("color"), QStringLiteral("Color"));
  QCOMPARE(entry->field("language"), QStringLiteral("English"));
  QCOMPARE(entry->field("certification"), QStringLiteral("PG (USA)"));
  QCOMPARE(entry->field("director"), QStringLiteral("George Miller"));
  QCOMPARE(set(entry->field("producer")), set(QStringLiteral("Geoff Burrowes; Michael Edgley; Simon Wincer")));
  QCOMPARE(entry->field("composer"), QStringLiteral("Bruce Rowland"));
  QCOMPARE(set(entry, "writer"), set("Cul Cullen; A.B. 'Banjo' Paterson;John Dixon"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QStringLiteral("Tom Burlinson::Jim Craig"));
  QCOMPARE(entry->field("imdb"), QStringLiteral("https://www.imdb.com/title/tt0084296/"));
  QVERIFY(!entry->field("imdb-rating").isEmpty());
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("plot").contains('>'));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  auto imageInfo = Tellico::ImageFactory::imageInfo(entry->field("cover"));
  QVERIFY(imageInfo.height() <= 256);
  QVERIFY(imageInfo.width() <= 256);
  QStringList altTitleList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("alttitle")));
  QVERIFY(altTitleList.contains(QStringLiteral("Herencia de un valiente")));
  QVERIFY(!entry->field("alttitle").contains(QStringLiteral("See more")));
}

void ImdbFetcherTest::testSnowyRiverFr() {
  QLocale::setDefault(QLocale(QLocale::French, QLocale::France));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "The Man From Snowy River");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QString::fromUtf8("L'homme de la rivière d'argent"));
  QCOMPARE(entry->field("year"), QLatin1String("1982"));
  // with GraphQL, genres still in english
//  QCOMPARE(set(entry->field("genre")), set(QLatin1String("Aventure; Drame; Romantique")));
//  QCOMPARE(entry->field("nationality"), QLatin1String("Australie"));
  QCOMPARE(entry->field("studio"), QLatin1String("Cambridge Productions; Michael Edgley International; Snowy River Investment Pty. Ltd."));
  QCOMPARE(entry->field("running-time"), QLatin1String("102"));
  QCOMPARE(entry->field("audio-track"), QLatin1String("Dolby"));
  QCOMPARE(entry->field("aspect-ratio"), QLatin1String("2.35 : 1"));
  QCOMPARE(entry->field("color"), QLatin1String("Color"));
//  QCOMPARE(entry->field("language"), QLatin1String("Anglais"));
  QCOMPARE(entry->field("director"), QLatin1String("George Miller"));
  QCOMPARE(entry->field("certification"), QLatin1String("PG (USA)"));
  QCOMPARE(set(entry->field("writer")), set(QLatin1String("Cul Cullen; A.B. 'Banjo' Paterson; John Dixon")));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QCOMPARE(castList.at(0), QLatin1String("Tom Burlinson::Jim Craig"));
  QCOMPARE(entry->field("imdb"), QLatin1String("https://www.imdb.com/title/tt0084296/"));
  QVERIFY(!entry->field("imdb-rating").isEmpty());
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("plot").contains(QStringLiteral("apos")));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field("cover").contains(QLatin1Char('/')));
}

void ImdbFetcherTest::testPacteDesLoupsEn() {
  m_config.writeEntry("System Locale", false);
  m_config.writeEntry("Custom Locale", "en_US");
  QLocale::setDefault(QLocale(QLocale::French, QLocale::France));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "Pacte des Loups");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QLatin1String("Brotherhood of the Wolf"));
}

void ImdbFetcherTest::testAsterix() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Astérix aux jeux olympiques"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  // title is returned in english
  QCOMPARE(entry->field("title"), QStringLiteral("Asterix at the Olympic Games"));
  QCOMPARE(entry->field("origtitle"), QString::fromUtf8("Astérix aux jeux olympiques"));
  QCOMPARE(set(entry, "director"), set(QString::fromUtf8("Thomas Langmann; Frédéric Forestier")));
  QCOMPARE(set(entry, "writer"), set(QString::fromUtf8("Franck Magnier; René Goscinny; Olivier Dazat; Alexandre Charlot; Thomas Langmann; Albert Uderzo")));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("plot").contains('>'));
  QVERIFY(!entry->field("plot").contains("»"));
  QStringList altTitleList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("alttitle")));
  QVERIFY(altTitleList.contains(QString::fromUtf8("Asterix at the Olympic Games")));
  QVERIFY(altTitleList.contains(QString::fromUtf8("Astérix en los Juegos Olímpicos")));
  QVERIFY(altTitleList.contains(QStringLiteral("Asterix alle Olimpiadi")));
}

// https://bugs.kde.org/show_bug.cgi?id=249096
void ImdbFetcherTest::testBodyDouble() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Body Double"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QEXPECT_FAIL("", "Movie title seems to be messed up", Continue);
  QCOMPARE(entry->field("title"), QStringLiteral("Body Double"));
  QCOMPARE(entry->field("director"), QStringLiteral("Brian De Palma"));
  QCOMPARE(set(entry, "writer"), set("Brian De Palma; Robert J. Avrech"));
  QCOMPARE(entry->field("producer"), QStringLiteral("Brian De Palma; Howard Gottfried"));
}

// https://bugs.kde.org/show_bug.cgi?id=249096
void ImdbFetcherTest::testMary() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("There's Something About Mary"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(set(entry, "director"), set("Peter Farrelly; Bobby Farrelly"));
  QCOMPARE(set(entry, "writer"), set("John J. Strauss; Ed Decter; Peter Farrelly; Bobby Farrelly"));
}

// https://bugs.kde.org/show_bug.cgi?id=262036
// https://bugs.kde.org/show_bug.cgi?id=401894
void ImdbFetcherTest::testOkunen() {
  // test bug 401894
  QLocale::setDefault(QLocale(QLocale::Italian, QLocale::Italy));
  m_config.writeEntry("Custom Fields", QStringLiteral("alttitle,imdb,imdb-rating"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("46-okunen no koi"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  // if the settings included origtitle, then the title would be "Big Bang Love, Juvenile A"
  QCOMPARE(entry->field("title"), QStringLiteral("46-okunen no koi"));
  QCOMPARE(entry->field("origtitle"), QString());
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Drama; Fantasy"));
  QCOMPARE(entry->field("director"), QStringLiteral("Takashi Miike"));
  // imdb rating is a volatile value, shouldn't be empty, shouldn't be 0
  QVERIFY(!entry->field("imdb-rating").isEmpty());
  QVERIFY(entry->field("imdb-rating") != QStringLiteral("0"));
  QCOMPARE(set(entry, "writer"), set("Ikki Kajiwara; Hisao Maki; Masa Nakamura"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QStringList altTitleList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("alttitle")));
  QVERIFY(altTitleList.contains(QStringLiteral("Big Bang Love, Juvenile A")));
  // return the custom fields setting to what the other tests use
  m_config.writeEntry("Custom Fields", QStringLiteral("alttitle,imdb,imdb-rating,origtitle"));
}

// https://bugs.kde.org/show_bug.cgi?id=314113
void ImdbFetcherTest::testFetchResultEncoding() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("jôbafuku onna harakiri"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  if(!hasNetwork()) {
    QSKIP("This test requires network access", SkipSingle);
    return;
  }

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->title(), QString::fromUtf8("'Shitsurakuen': Jôbafuku Onna Harakiri"));
}

// https://bugs.kde.org/show_bug.cgi?id=336765
void ImdbFetcherTest::testBabel() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Babel"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("Babel"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("director"), QString::fromUtf8("Alejandro G. Iñárritu"));
  QCOMPARE(set(entry, "writer"), set(QString::fromUtf8("Alejandro G. Iñárritu; Guillermo Arriaga")));
  QCOMPARE(set(entry, "producer"), set(QString::fromUtf8("Steve Golin; Alejandro G. Iñárritu; Jimmy Abounouom; Jon Kilik; Kay Ueda; Norihisa Harada; Ann Ruark; Corinne Golden Weber; Tita Lombardo")));
}

void ImdbFetcherTest::testFirefly() {
  m_config.writeEntry("Custom Fields", QStringLiteral("imdb,episode"));
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("firefly 2002"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("Firefly"));
  QCOMPARE(entry->field("year"), QStringLiteral("2002"));
  QCOMPARE(set(entry->field("producer")), set(QStringLiteral("Gareth Davies; Lisa Lassek; Brian Wankum; Tim Minear; Joss Whedon")));
  QVERIFY(entry->field("director").contains(QStringLiteral("Joss Whedon")));
  QVERIFY(entry->field("director").contains(QStringLiteral("Tim Minear")));
  QVERIFY(entry->field("writer").contains(QStringLiteral("Joss Whedon")));
  QVERIFY(entry->field("writer").contains(QStringLiteral("Cheryl Cain")));
  QCOMPARE(entry->field("composer"), QStringLiteral("Greg Edmonson"));
  QCOMPARE(set(entry->field("genre")), set(QStringLiteral("Adventure; Drama; Sci-Fi")));
  QVERIFY(entry->field("cast").startsWith(QStringLiteral("Nathan Fillion::Captain Malcolm 'Mal' Reynolds")));
  QVERIFY(!entry->field("cast").contains(QStringLiteral("episodes")));
  QStringList episodeList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("episode")));
  QVERIFY(!episodeList.isEmpty());
  if(!episodeList.isEmpty()) QCOMPARE(episodeList.at(0), QStringLiteral("The Train Job::1::1"));
  QVERIFY(entry->field("plot").startsWith(QStringLiteral("Five hundred")));
}

void ImdbFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::VideoCollection(true));
  coll->addField(Tellico::Data::Field::createDefaultField(Tellico::Data::Field::ImdbField));
  Tellico::Data::EntryPtr emptyEntry(new Tellico::Data::Entry(coll));
  emptyEntry->setField(QLatin1String("imdb"), QStringLiteral("https://www.imdb.com/title/tt0084296/"));

  m_config.writeEntry("Custom Fields", QStringLiteral("imdb,episode"));
  Tellico::Fetch::IMDBFetcher fetcher(this);
  fetcher.readConfig(m_config);
  auto request = fetcher.updateRequest(emptyEntry);
  request.setCollectionType(coll->type());

  Tellico::Data::EntryList results = DO_FETCH1(&fetcher, request, 1);
  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("title"), QStringLiteral("The Man from Snowy River"));
  QCOMPARE(entry->field("year"), QStringLiteral("1982"));
  QCOMPARE(entry->field("imdb"), QStringLiteral("https://www.imdb.com/title/tt0084296/"));
}
