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

#include "../translators/deliciousimporter.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/musiccollection.h"
#include "../collections/gamecollection.h"
#include "../collectionfactory.h"
#include "../filter.h"
#include "../fieldformat.h"
#include "../utils/datafileregistry.h"

#include <KLocalizedString>

#include <QTest>

QTEST_GUILESS_MAIN( DeliciousTest )

void DeliciousTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/delicious2tellico.xsl"));
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  Tellico::RegisterCollection<Tellico::Data::GameCollection> registerGame(Tellico::Data::Collection::Game, "game");
}

void DeliciousTest::testBooks1() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/delicious1_books.xml"));
  Tellico::Import::DeliciousImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Book));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 5);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Lost in Translation"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("1998"));
  QCOMPARE(entry->field("author"), QStringLiteral("Nicole Mones; Robby Stephenson"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Delacorte Press"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("0-385-31934-7"));
  QCOMPARE(entry->field("binding"), QStringLiteral("Hardback"));
  QCOMPARE(entry->field("keyword"), QStringLiteral("United States; Contemporary & Robby"));
  QCOMPARE(entry->field("pages"), QStringLiteral("384"));
  QCOMPARE(entry->field("rating"), QStringLiteral("4"));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("$23.95"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("07-08-2006"));
  QVERIFY(entry->field("comments").startsWith(QStringLiteral("<p><span style=\"font-size:12pt;\">Nicole Mones doesn't")));
}

void DeliciousTest::testBooks2() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/delicious2_books.xml"));
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 7);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Restaurant at the End of the Universe"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("0-517-54535-7"));
  QCOMPARE(entry->field("cdate"), QStringLiteral("2007-12-19"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2009-06-11"));
  const auto authors = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("author")));
  QCOMPARE(authors.count(), 1);
  QCOMPARE(authors.first(), QStringLiteral("Douglas Adams"));
  QCOMPARE(entry->field("binding"), QStringLiteral("Hardback"));
  QCOMPARE(entry->field("rating"), QStringLiteral("4.5")); // visually, this gets shown as 4 stars
  QCOMPARE(entry->field("pages"), QStringLiteral("250"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("1982"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Harmony"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("2007-12-18"));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("$12.95"));
  QCOMPARE(entry->field("signed"), QStringLiteral("true"));
  QCOMPARE(entry->field("condition"), QStringLiteral("Used"));
}

void DeliciousTest::testMovies1() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/delicious1_movies.xml"));
  Tellico::Import::DeliciousImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Video));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 4);

  // first a movie
  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Driving Miss Daisy"));
  QCOMPARE(entry->field("year"), QStringLiteral("1990"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("USA"));
  QCOMPARE(entry->field("director"), QStringLiteral("Bruce Beresford"));
  QStringList cast = QStringList() << QStringLiteral("Morgan Freeman") << QStringLiteral("Jessica Tandy") << QStringLiteral("Dan Aykroyd") << QStringLiteral("Patti LuPone") << QStringLiteral("Esther Rolle");
  QCOMPARE(entry->field("cast"), cast.join(Tellico::FieldFormat::rowDelimiterString()));
  QCOMPARE(entry->field("format"), QStringLiteral("NTSC"));
  QCOMPARE(entry->field("medium"), QStringLiteral("DVD"));
  QCOMPARE(entry->field("color"), QStringLiteral("Color"));
  QCOMPARE(entry->field("aspect-ratio"), QStringLiteral("1.85:1"));
  QCOMPARE(entry->field("audio-track"), QStringLiteral("Dolby"));
  QCOMPARE(entry->field("widescreen"), QStringLiteral("true"));
  QCOMPARE(entry->field("running-time"), QStringLiteral("99"));
  QCOMPARE(entry->field("certification"), QStringLiteral("PG (USA)"));
  QCOMPARE(entry->field("region"), QStringLiteral("Region 1"));
  QCOMPARE(entry->field("rating"), QStringLiteral("4.5"));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("$14.98"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("25-03-2006"));
  QVERIFY(entry->field("keyword").startsWith(QStringLiteral("Period Piece; Race Relations")));

  // check the TV show, too
  entry = coll->entryById(4);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("South Park - The Complete Sixth Season"));
  QCOMPARE(entry->field("year"), QStringLiteral("1997"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("USA"));
  QCOMPARE(entry->field("studio"), QStringLiteral("Comedy Central"));
  QCOMPARE(entry->field("director"), QStringLiteral("Trey Parker; Matt Stone"));
  // the shelf name gets added to keyword list
  QVERIFY(entry->field("keyword").contains(QStringLiteral("TV Shows")));

  Tellico::FilterList filters = coll->filters();
  QCOMPARE(filters.count(), 1);

  Tellico::FilterPtr filter = filters.first();
  QVERIFY(filter);
  QCOMPARE(filter->name(), QStringLiteral("TV Shows"));
  QVERIFY(filter->matches(entry));
}

void DeliciousTest::testMovies2() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/delicious2_movies.xml"));
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 4);

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("2001 - A Space Odyssey"));
  QCOMPARE(entry->field("certification"), QStringLiteral("G (USA)"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("USA"));
  QCOMPARE(entry->field("aspect-ratio"), QStringLiteral("2.35:1"));
  QCOMPARE(entry->field("audio-track"), QStringLiteral("Dolby"));
  QCOMPARE(entry->field("widescreen"), QStringLiteral("true"));
  QCOMPARE(entry->field("director"), QStringLiteral("Stanley Kubrick"));
  QCOMPARE(entry->field("color"), QStringLiteral("Color"));
  QCOMPARE(entry->field("format"), QStringLiteral("NTSC"));
  QCOMPARE(entry->field("medium"), QStringLiteral("DVD"));
  QCOMPARE(entry->field("running-time"), QStringLiteral("148"));
  QCOMPARE(entry->field("rating"), QStringLiteral("4"));
  QCOMPARE(entry->field("year"), QStringLiteral("1968"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("2007-12-19"));
  QCOMPARE(entry->field("cdate"), QStringLiteral("2007-12-20"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2009-06-11"));

  entry = coll->entryById(4);
  QVERIFY(entry);
  QCOMPARE(entry->field("region"), QStringLiteral("Region 1"));
}

void DeliciousTest::testMusic1() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/delicious1_music.xml"));
  Tellico::Import::DeliciousImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Album));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 3);

  // first a movie
  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Are You Listening?"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Dolores O'Riordan"));
  QCOMPARE(entry->field("year"), QStringLiteral("2007"));
  QCOMPARE(entry->field("medium"), QStringLiteral("Compact Disc"));
  QCOMPARE(entry->field("label"), QStringLiteral("Sanctuary Records"));
  const auto tracks = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(tracks.count(), 12);
  QCOMPARE(tracks.first(), QStringLiteral("Ordinary Day"));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("$15.98"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("27-06-2008"));
  QCOMPARE(entry->field("rating"), QStringLiteral("4.5"));
}

void DeliciousTest::testMusic2() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/delicious2_music.xml"));
  Tellico::Import::DeliciousImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 3);

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Ultimate Sin"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Ozzy Osbourne"));
  QCOMPARE(entry->field("year"), QStringLiteral("1987"));
  QCOMPARE(entry->field("medium"), QStringLiteral("Compact Disc"));
  QCOMPARE(entry->field("label"), QStringLiteral("Epic Aus/Zoom"));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("$15.98"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("2009-01-17"));
  QCOMPARE(entry->field("rating"), QStringLiteral("3.5"));
  QCOMPARE(entry->field("keyword"), QStringLiteral("Hard Rock & Metal; Rock"));
}

void DeliciousTest::testGames1() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/delicious1_games.xml"));
  Tellico::Import::DeliciousImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Game));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Game);
  QCOMPARE(coll->entryCount(), 2);

  // first a movie
  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Spider-Man 2: The Movie 2"));
  QCOMPARE(entry->field("certification"), QStringLiteral("Teen"));
  QCOMPARE(entry->field("platform"), QStringLiteral("GameCube"));
  QCOMPARE(entry->field("year"), QStringLiteral("2004"));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("$49.99"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("25-03-2006"));
  QCOMPARE(entry->field("rating"), QStringLiteral("4.5"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Activision"));
}
