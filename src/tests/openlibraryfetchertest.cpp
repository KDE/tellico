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

#include "openlibraryfetchertest.h"

#include "../fetch/openlibraryfetcher.h"
#include "../entry.h"
#include "../collections/bookcollection.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( OpenLibraryFetcherTest )

OpenLibraryFetcherTest::OpenLibraryFetcherTest() : AbstractFetcherTest() {
}

void OpenLibraryFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void OpenLibraryFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("Barrayar"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Barrayar"));
}

void OpenLibraryFetcherTest::testAuthor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Person,
                                       QStringLiteral("Bujold"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry->field(QStringLiteral("author")).contains(QStringLiteral("Lois McMaster Bujold")));
}

void OpenLibraryFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0789312239"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("This is Venice"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("M. Sasek"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-7893-1223-9"));
  QCOMPARE(entry->field(QStringLiteral("lccn")), QStringLiteral("2004110229"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2005"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Juvenile literature."));
  QCOMPARE(entry->field(QStringLiteral("keyword")), QStringLiteral("Venice (Italy) -- Description and travel -- Juvenile literature"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Universe"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("English"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("56"));
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
}

// also see https://bugs.kde.org/show_bug.cgi?id=504586
void OpenLibraryFetcherTest::testIsbn13() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("9791030705966"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  // sometimes OpenLibrary has multiple works defined, so just verify one or more
  QVERIFY(results.size() >= 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Le Syndrome Magneto"));
  QCOMPARE(entry->field(QStringLiteral("subtitle")), QStringLiteral("Et si les méchants avaient raison ?"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Au diable vauvert"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("1-03070-596-8"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("448"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Benjamin Patinaud"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("French / français"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2023"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QVERIFY(entry->field(QStringLiteral("openlibrary-work")).isEmpty()); // temporary field should not exist
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void OpenLibraryFetcherTest::testLccn() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("OpenLibrary"));
  cg.writeEntry("Custom Fields", QStringLiteral("openlibrary"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::LCCN,
                                       QStringLiteral("2004110229"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("This is Venice"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("M. Sasek"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-7893-1223-9"));
  QCOMPARE(entry->field(QStringLiteral("lccn")), QStringLiteral("2004110229"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2005"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Juvenile literature."));
  QCOMPARE(entry->field(QStringLiteral("keyword")), QStringLiteral("Venice (Italy) -- Description and travel -- Juvenile literature"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Universe"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("English"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("56"));
  QCOMPARE(entry->field(QStringLiteral("openlibrary")), QStringLiteral("https://openlibrary.org/books/OL3315616M"));
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
}

void OpenLibraryFetcherTest::testMultipleIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("0789312239; 9780596000486"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 2);
}

void OpenLibraryFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true));
  Tellico::Data::EntryPtr oldEntry(new Tellico::Data::Entry(coll));
  coll->addEntries(oldEntry);

  oldEntry->setField(QStringLiteral("title"), QStringLiteral("The Great Hunt"));
  oldEntry->setField(QStringLiteral("publisher"), QStringLiteral("Orbit"));

  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));
  auto f = static_cast<Tellico::Fetch::OpenLibraryFetcher*>(fetcher.data());

  auto request = f->updateRequest(oldEntry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  request.setCollectionType(coll->type());
  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("The Great Hunt"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2014"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Orbit"));
}

void OpenLibraryFetcherTest::testComic() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::ISBN,
                                       QStringLiteral("4048690663"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::OpenLibraryFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  // sometimes OpenLibrary has multiple works defined, so just verify one or more
  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::ComicBook);
  QCOMPARE(entry->field(QStringLiteral("title")), QString::fromUtf8("よつばと！ 1"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("4-04869066-3"));
}
