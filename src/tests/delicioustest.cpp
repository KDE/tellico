/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "delicioustest.h"
#include "qtest_kde.h"

#include "../translators/deliciousimporter.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/musiccollection.h"
#include "../collections/gamecollection.h"
#include "../collectionfactory.h"
#include "../filter.h"
#include "../fieldformat.h"

#include <kstandarddirs.h>

#define FIELDS(entry, fieldName) Tellico::FieldFormat::splitValue(entry->field(fieldName))
#define ROWS(entry, fieldName) Tellico::FieldFormat::splitTable(entry->field(fieldName))

QTEST_KDEMAIN_CORE( DeliciousTest )

void DeliciousTest::initTestCase() {
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  Tellico::RegisterCollection<Tellico::Data::GameCollection> registerGame(Tellico::Data::Collection::Game, "game");
}

void DeliciousTest::testBooks1() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/delicious1_books.xml");
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 5);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("Lost in Translation"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("1998"));
  QCOMPARE(entry->field("author"), QLatin1String("Nicole Mones; Robby Stephenson"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Delacorte Press"));
  QCOMPARE(entry->field("isbn"), QLatin1String("0385319347"));
  QCOMPARE(entry->field("binding"), QLatin1String("Hardback"));
  QCOMPARE(entry->field("keyword"), QLatin1String("United States; Contemporary & Robby"));
  QCOMPARE(entry->field("pages"), QLatin1String("384"));
  QCOMPARE(entry->field("rating"), QLatin1String("4"));
  QCOMPARE(entry->field("pur_price"), QLatin1String("$23.95"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("07-08-2006"));
  QVERIFY(entry->field("comments").startsWith(QLatin1String("<p><span style=\"font-size:12pt;\">Nicole Mones doesn't")));
}

void DeliciousTest::testBooks2() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/delicious2_books.xml");
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 7);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("The Restaurant at the End of the Universe"));
  QCOMPARE(entry->field("isbn"), QLatin1String("0517545357"));
  QCOMPARE(entry->field("cdate"), QLatin1String("2007-12-19"));
  QCOMPARE(entry->field("mdate"), QLatin1String("2009-06-11"));
  QCOMPARE(FIELDS(entry, "author").count(), 1);
  QCOMPARE(FIELDS(entry, "author").first(), QLatin1String("Douglas Adams"));
  QCOMPARE(entry->field("binding"), QLatin1String("Hardback"));
  QCOMPARE(entry->field("rating"), QLatin1String("4.5")); // visually, this gets shown as 4 stars
  QCOMPARE(entry->field("pages"), QLatin1String("250"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("1982"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Harmony"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("2007-12-18"));
  QCOMPARE(entry->field("pur_price"), QLatin1String("$12.95"));
  QCOMPARE(entry->field("signed"), QLatin1String("true"));
  QCOMPARE(entry->field("condition"), QLatin1String("Used"));
}

void DeliciousTest::testMovies1() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/delicious1_movies.xml");
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 4);

  // first a movie
  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("Driving Miss Daisy"));
  QCOMPARE(entry->field("year"), QLatin1String("1990"));
  QCOMPARE(entry->field("nationality"), QLatin1String("USA"));
  QCOMPARE(entry->field("director"), QLatin1String("Bruce Beresford"));
  QStringList cast = QStringList() << "Morgan Freeman" << "Jessica Tandy" << "Dan Aykroyd" << "Patti LuPone" << "Esther Rolle";
  QCOMPARE(entry->field("cast"), cast.join(Tellico::FieldFormat::rowDelimiterString()));
  QCOMPARE(entry->field("format"), QLatin1String("NTSC"));
  QCOMPARE(entry->field("medium"), QLatin1String("DVD"));
  QCOMPARE(entry->field("color"), QLatin1String("Color"));
  QCOMPARE(entry->field("aspect-ratio"), QLatin1String("1.85:1"));
  QCOMPARE(entry->field("audio-track"), QLatin1String("Dolby"));
  QCOMPARE(entry->field("widescreen"), QLatin1String("true"));
  QCOMPARE(entry->field("running-time"), QLatin1String("99"));
  QCOMPARE(entry->field("certification"), QLatin1String("PG (USA)"));
  QCOMPARE(entry->field("region"), QLatin1String("Region 1"));
  QCOMPARE(entry->field("rating"), QLatin1String("4.5"));
  QCOMPARE(entry->field("pur_price"), QLatin1String("$14.98"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("25-03-2006"));
  QVERIFY(entry->field("keyword").startsWith(QLatin1String("Period Piece; Race Relations")));

  // check the TV show, too
  entry = coll->entryById(4);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("South Park - The Complete Sixth Season"));
  QCOMPARE(entry->field("year"), QLatin1String("1997"));
  QCOMPARE(entry->field("nationality"), QLatin1String("USA"));
  QCOMPARE(entry->field("studio"), QLatin1String("Comedy Central"));
  QCOMPARE(entry->field("director"), QLatin1String("Trey Parker; Matt Stone"));
  // the shelf name gets added to keyword list
  QVERIFY(entry->field("keyword").contains(QLatin1String("TV Shows")));

  Tellico::FilterList filters = coll->filters();
  QCOMPARE(filters.count(), 1);

  Tellico::FilterPtr filter = filters.first();
  QVERIFY(filter);
  QCOMPARE(filter->name(), QLatin1String("TV Shows"));
  QVERIFY(filter->matches(entry));
}

void DeliciousTest::testMovies2() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/delicious2_movies.xml");
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 4);

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("2001 - A Space Odyssey"));
  QCOMPARE(entry->field("certification"), QLatin1String("G (USA)"));
  QCOMPARE(entry->field("nationality"), QLatin1String("USA"));
  QCOMPARE(entry->field("aspect-ratio"), QLatin1String("2.35:1"));
  QCOMPARE(entry->field("audio-track"), QLatin1String("Dolby"));
  QCOMPARE(entry->field("widescreen"), QLatin1String("true"));
  QCOMPARE(entry->field("director"), QLatin1String("Stanley Kubrick"));
  QCOMPARE(entry->field("color"), QLatin1String("Color"));
  QCOMPARE(entry->field("format"), QLatin1String("NTSC"));
  QCOMPARE(entry->field("medium"), QLatin1String("DVD"));
  QCOMPARE(entry->field("running-time"), QLatin1String("148"));
  QCOMPARE(entry->field("rating"), QLatin1String("4"));
  QCOMPARE(entry->field("year"), QLatin1String("1968"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("2007-12-19"));
  QCOMPARE(entry->field("cdate"), QLatin1String("2007-12-20"));
  QCOMPARE(entry->field("mdate"), QLatin1String("2009-06-11"));

  entry = coll->entryById(4);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("region"), QLatin1String("Region 1"));
}

void DeliciousTest::testMusic1() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/delicious1_music.xml");
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 3);

  // first a movie
  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("Are You Listening?"));
  QCOMPARE(entry->field("artist"), QLatin1String("Dolores O'Riordan"));
  QCOMPARE(entry->field("year"), QLatin1String("2007"));
  QCOMPARE(entry->field("medium"), QLatin1String("Compact Disc"));
  QCOMPARE(entry->field("label"), QLatin1String("Sanctuary Records"));
  QCOMPARE(ROWS(entry, "track").count(), 12);
  QCOMPARE(ROWS(entry, "track").first(), QLatin1String("Ordinary Day"));
  QCOMPARE(entry->field("pur_price"), QLatin1String("$15.98"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("27-06-2008"));
  QCOMPARE(entry->field("rating"), QLatin1String("4.5"));
}

void DeliciousTest::testMusic2() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/delicious2_music.xml");
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 3);

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("The Ultimate Sin"));
  QCOMPARE(entry->field("artist"), QLatin1String("Ozzy Osbourne"));
  QCOMPARE(entry->field("year"), QLatin1String("1987"));
  QCOMPARE(entry->field("medium"), QLatin1String("Compact Disc"));
  QCOMPARE(entry->field("label"), QLatin1String("Epic Aus/Zoom"));
  QCOMPARE(entry->field("pur_price"), QLatin1String("$15.98"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("2009-01-17"));
  QCOMPARE(entry->field("rating"), QLatin1String("3.5"));
  QCOMPARE(entry->field("keyword"), QLatin1String("Hard Rock & Metal; Rock"));
}

void DeliciousTest::testGames1() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/delicious1_games.xml");
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Game);
  QCOMPARE(coll->entryCount(), 2);

  // first a movie
  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("Spider-Man 2: The Movie 2"));
  QCOMPARE(entry->field("certification"), QLatin1String("Teen"));
  QCOMPARE(entry->field("platform"), QLatin1String("GameCube"));
  QCOMPARE(entry->field("year"), QLatin1String("2004"));
  QCOMPARE(entry->field("pur_price"), QLatin1String("$49.99"));
  QCOMPARE(entry->field("pur_date"), QLatin1String("25-03-2006"));
  QCOMPARE(entry->field("rating"), QLatin1String("4.5"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Activision"));
}
