/***************************************************************************
    Copyright (C) 2011-2020 Robby Stephenson <robby@periapsis.org>
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

#include "doubanfetchertest.h"

#include "../fetch/doubanfetcher.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/musiccollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( DoubanFetcherTest )

DoubanFetcherTest::DoubanFetcherTest() : AbstractFetcherTest() {
}

void DoubanFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection>  registerBook(Tellico::Data::Collection::Book,   "book");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  Tellico::ImageFactory::init();

  m_config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("douban"));
  m_config.writeEntry("Custom Fields", QStringLiteral("douban,origtitle"));
}

void DoubanFetcherTest::testBookTitle() {
  auto f = new Tellico::Fetch::DoubanFetcher(this);
  Tellico::Fetch::Fetcher::Ptr fetcher(f);
  if(1) {
    QUrl testUrl1 = QUrl::fromLocalFile(QFINDTESTDATA("data/douban_book_search.json"));
    QUrl testUrl2 = QUrl::fromLocalFile(QFINDTESTDATA("data/douban_book_details.json"));
    f->setTestUrl1(testUrl1);
    f->setTestUrl2(testUrl2);
  }
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Keyword,
                                       QStringLiteral("大设计 列纳德·蒙洛迪诺"));
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Book);

  QCOMPARE(entry->field("title"), QString::fromUtf8("大设计"));
  QCOMPARE(entry->field("author"), QString::fromUtf8("[英] 斯蒂芬·霍金; 列纳德·蒙洛迪诺"));
  QCOMPARE(entry->field("translator"), QString::fromUtf8("吴忠超"));
  QCOMPARE(entry->field("publisher"), QString::fromUtf8("湖南科学技术出版社"));
  QCOMPARE(entry->field("binding"), QStringLiteral("Hardback"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("2011"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("7-53576544-0"));
  QCOMPARE(entry->field("pages"), QStringLiteral("176"));
  QVERIFY(!entry->field(QStringLiteral("keyword")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void DoubanFetcherTest::testISBN() {
  auto f = new Tellico::Fetch::DoubanFetcher(this);
  Tellico::Fetch::Fetcher::Ptr fetcher(f);
  if(1) {
    QUrl testUrl = QUrl::fromLocalFile(QFINDTESTDATA("data/douban_book_isbn.json"));
    f->setTestUrl1(testUrl);
  }
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("9787535765444"));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Book);

  QCOMPARE(entry->field("title"), QString::fromUtf8("大设计"));
  QCOMPARE(entry->field("author"), QString::fromUtf8("[英] 斯蒂芬·霍金; 列纳德·蒙洛迪诺"));
  QCOMPARE(entry->field("translator"), QString::fromUtf8("吴忠超"));
  QCOMPARE(entry->field("publisher"), QString::fromUtf8("湖南科学技术出版社"));
  QCOMPARE(entry->field("binding"), QStringLiteral("Hardback"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("2011"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("7-53576544-0"));
  QCOMPARE(entry->field("pages"), QStringLiteral("176"));
  QCOMPARE(entry->field("origtitle"), QStringLiteral("The Grand Design"));
  QCOMPARE(entry->field("douban"), QStringLiteral("https://book.douban.com/subject/5422665/"));
  QVERIFY(!entry->field(QStringLiteral("keyword")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void DoubanFetcherTest::testVideo() {
  auto f = new Tellico::Fetch::DoubanFetcher(this);
  Tellico::Fetch::Fetcher::Ptr fetcher(f);
  if(1) {
    QUrl testUrl1 = QUrl::fromLocalFile(QFINDTESTDATA("data/douban_movie_search.json"));
    QUrl testUrl2 = QUrl::fromLocalFile(QFINDTESTDATA("data/douban_movie_details.json"));
    f->setTestUrl1(testUrl1);
    f->setTestUrl2(testUrl2);
  }
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword,
                                       QStringLiteral("钢铁侠2"));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Video);

  QCOMPARE(entry->field("title"), QString::fromUtf8("钢铁侠2"));
  QCOMPARE(entry->field("origtitle"), QStringLiteral("Iron Man 2"));
  QCOMPARE(entry->field("year"), QStringLiteral("2010"));
  QCOMPARE(entry->field("director"), QString::fromUtf8("乔恩·费儒"));
//  QCOMPARE(entry->field("writer"), QString::fromUtf8("贾斯汀·塞洛克斯"));
//  QCOMPARE(entry->field("running-time"), QStringLiteral("124"));
  QVERIFY(!entry->field(QStringLiteral("genre")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cast")).isEmpty());
//  QVERIFY(!entry->field(QStringLiteral("nationality")).isEmpty());
//  QVERIFY(!entry->field(QStringLiteral("language")).isEmpty());
//  QVERIFY(!entry->field(QStringLiteral("keyword")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void DoubanFetcherTest::testMusic() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QLatin1String("Top Gun Original Motion Picture"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DoubanFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Album);

  QCOMPARE(entry->field("title"), QStringLiteral("TOP GUN/SOUNDTRACK"));
  QCOMPARE(entry->field("year"), QStringLiteral("2013"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Original Motion Picture Soundtrack"));
  QCOMPARE(entry->field("label"), QString::fromUtf8("索尼音乐"));
//  QCOMPARE(entry->field("medium"), QStringLiteral("Compact Disc"));
  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.count(), 10);
  QCOMPARE(trackList.front(), QStringLiteral("Danger Zone::Kenny Loggins"));
//  QVERIFY(!entry->field(QStringLiteral("keyword")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void DoubanFetcherTest::testMusicAdele() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QLatin1String("Adele 21"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DoubanFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Album);

  QCOMPARE(entry->field("title"), QStringLiteral("21"));
  QCOMPARE(entry->field("year"), QStringLiteral("2011"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Adele"));
  QCOMPARE(entry->field("medium"), QStringLiteral("Compact Disc"));
  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.count(), 12);
  QCOMPARE(trackList.front(), QStringLiteral("Rolling In the Deep::Adele::3:48"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void DoubanFetcherTest::testMusicArtPepper() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QLatin1String("Art Pepper Meets the Rhythm Section"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DoubanFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Album);

  QCOMPARE(entry->field("title"), QStringLiteral("Art Pepper Meets The Rhythm Section"));
  QCOMPARE(entry->field("year"), QStringLiteral("1991"));
  QCOMPARE(entry->field("label"), QStringLiteral("Ojc"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Art Pepper"));
  QCOMPARE(entry->field("medium"), QStringLiteral("Compact Disc"));
  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.count(), 9);
  // doesn't seem to have the duration
  QVERIFY(trackList.front().startsWith(QStringLiteral("You'd Be So Nice To Come Home To::Art Pepper")));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
