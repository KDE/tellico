/***************************************************************************
    Copyright (C) 2025 Robby Stephenson <robby@periapsis.org>
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

#include "onmyshelftest.h"

#include "../translators/onmyshelfimporter.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/musiccollection.h"
#include "../collectionfactory.h"
#include "../fieldformat.h"

#include <KLocalizedString>

#include <QTest>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( OnMyShelfTest )

void OnMyShelfTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
}

void OnMyShelfTest::testBooks() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/onmyshelf-books.json"));
  Tellico::Import::OnMyShelfImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Book));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 2);
  QCOMPARE(coll->title(), QStringLiteral("My books"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("title"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("1999"));
  QCOMPARE(entry->field("author"), QStringLiteral("my author1; my author2"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("publisher"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("0-671-57849-9"));
  QCOMPARE(entry->field("binding"), QStringLiteral("Paperback"));
  QCOMPARE(entry->field("series"), QStringLiteral("series"));
  QCOMPARE(entry->field("series_num"), QStringLiteral("1"));
  QCOMPARE(entry->field("read"), QStringLiteral("true"));
  QCOMPARE(entry->field("genre"), QStringLiteral("genre"));
  QCOMPARE(entry->field("cdate"), QStringLiteral("2025-08-30"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2025-08-30"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(entry->field("plot").contains(QLatin1String("<br/>"))); // \n is converted

  const auto borrowers = coll->borrowers();
  QCOMPARE(borrowers.count(), 1);
  const auto borrower = borrowers.at(0);
  QVERIFY(borrower);
  QCOMPARE(borrower->name(), QStringLiteral("my borrower"));

  const auto loans = borrower->loans();
  QCOMPARE(loans.count(), 1);
  const auto loan = loans.at(0);
  QVERIFY(loan);
  QCOMPARE(loan->loanDate(), QDate::fromString("2025-08-31", Qt::ISODate));
  QCOMPARE(loan->note(), QStringLiteral("loaned this time"));
  QVERIFY(loan->entry());
  QCOMPARE(loan->entry()->id(), entry->id());
}

void OnMyShelfTest::testMovies() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/onmyshelf-movies.json"));
  Tellico::Import::OnMyShelfImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Video));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 1);
  QCOMPARE(coll->title(), QStringLiteral("My movies"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("title"));
  QCOMPARE(entry->field("director"), QStringLiteral("my director"));
  QCOMPARE(entry->field("rating"), QStringLiteral("1"));
  // not sure what format should be
  QCOMPARE(entry->field("cdate"), QStringLiteral("2025-08-31"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2025-08-31"));
  QCOMPARE(entry->field("plot"), QStringLiteral("plot here"));
}

void OnMyShelfTest::testComics() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/onmyshelf-comics.json"));
  Tellico::Import::OnMyShelfImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::ComicBook));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::ComicBook);
  QCOMPARE(coll->entryCount(), 1);
  QCOMPARE(coll->title(), QStringLiteral("My comics"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("title"));
  QCOMPARE(entry->field("writer"), QStringLiteral("my author"));
  QCOMPARE(entry->field("series"), QStringLiteral("series"));
  QCOMPARE(entry->field("issue"), QStringLiteral("1"));
  // not sure about format
  QCOMPARE(entry->field("cdate"), QStringLiteral("2025-08-31"));
  QCOMPARE(entry->field("mdate"), QStringLiteral("2025-08-31"));
  QCOMPARE(entry->field("plot"), QStringLiteral("synopsis"));
}

void OnMyShelfTest::testBoardGames() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/onmyshelf-boardgames.json"));
  Tellico::Import::OnMyShelfImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::BoardGame));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::BoardGame);
  QCOMPARE(coll->entryCount(), 1);
  QCOMPARE(coll->title(), QStringLiteral("My board games"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("name"));
  QCOMPARE(entry->field("mechanism"), QStringLiteral("mechanism"));
  QCOMPARE(entry->field("designer"), QStringLiteral("my author"));
  QCOMPARE(entry->field("minimum-age"), QStringLiteral("7"));
  // skip editor, illustrator, language
}
