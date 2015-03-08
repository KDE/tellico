/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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
#include "qtest_kde.h"

#include "../fetch/doubanfetcher.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/musiccollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KStandardDirs>

QTEST_KDEMAIN( DoubanFetcherTest, GUI )

DoubanFetcherTest::DoubanFetcherTest() : AbstractFetcherTest() {
}

void DoubanFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection>  registerBook(Tellico::Data::Collection::Book,   "book");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  // since we use the xslt importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();
}

void DoubanFetcherTest::testBookTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Keyword,
                                       QString::fromUtf8("大设计 列纳德·蒙洛迪诺"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DoubanFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Book);

  QCOMPARE(entry->field("title"), QString::fromUtf8("大设计"));
  QCOMPARE(entry->field("author"), QString::fromUtf8("[英] 斯蒂芬·霍金; 列纳德·蒙洛迪诺"));
  QCOMPARE(entry->field("translator"), QString::fromUtf8("吴忠超"));
  QCOMPARE(entry->field("publisher"), QString::fromUtf8("湖南科学技术出版社"));
  QCOMPARE(entry->field("binding"), QLatin1String("Hardback"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("2011"));
  QCOMPARE(entry->field("isbn"), QLatin1String("978-7-53576544-4"));
  QCOMPARE(entry->field("pages"), QLatin1String("176"));
  QVERIFY(!entry->field(QLatin1String("keyword")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("comments")).isEmpty());
}

void DoubanFetcherTest::testISBN() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Keyword,
                                       QLatin1String("9787535765444"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DoubanFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Book);

  QCOMPARE(entry->field("title"), QString::fromUtf8("大设计"));
  QCOMPARE(entry->field("author"), QString::fromUtf8("[英] 斯蒂芬·霍金; 列纳德·蒙洛迪诺"));
  QCOMPARE(entry->field("translator"), QString::fromUtf8("吴忠超"));
  QCOMPARE(entry->field("publisher"), QString::fromUtf8("湖南科学技术出版社"));
  QCOMPARE(entry->field("binding"), QLatin1String("Hardback"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("2011"));
  QCOMPARE(entry->field("isbn"), QLatin1String("978-7-53576544-4"));
  QCOMPARE(entry->field("pages"), QLatin1String("176"));
  QVERIFY(!entry->field(QLatin1String("keyword")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("comments")).isEmpty());
}

void DoubanFetcherTest::testVideo() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword,
                                       QString::fromUtf8("钢铁侠2"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DoubanFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Video);

  QCOMPARE(entry->field("title"), QLatin1String("Iron Man 2"));
  QCOMPARE(entry->field("year"), QLatin1String("2010"));
  QCOMPARE(entry->field("director"), QString::fromUtf8("乔恩·费儒"));
  QCOMPARE(entry->field("running-time"), QLatin1String("124"));
  QVERIFY(!entry->field(QLatin1String("genre")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cast")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("nationality")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("language")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("keyword")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
}

void DoubanFetcherTest::testMusic() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Keyword,
                                       QString::fromUtf8("Top Gun Original Motion Picture"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DoubanFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Album);

  QCOMPARE(entry->field("title"), QLatin1String("Top Gun: Original Motion Picture Soundtrack"));
  QCOMPARE(entry->field("year"), QLatin1String("1990"));
  QCOMPARE(entry->field("artist"), QLatin1String("Various Artists"));
  QCOMPARE(entry->field("label"), QLatin1String("Sony"));
  QCOMPARE(entry->field("medium"), QLatin1String("Compact Disc"));
  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field("track"));
  QCOMPARE(trackList.count(), 10);
  QCOMPARE(trackList.front(), QLatin1String("Danger Zone::Kenny Loggins"));
  QVERIFY(!entry->field(QLatin1String("keyword")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
//  QVERIFY(!entry->field(QLatin1String("comments")).isEmpty());
}
