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

#include "ibsfetchertest.h"

#include "../fetch/ibsfetcher.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <QTest>

QTEST_GUILESS_MAIN( IBSFetcherTest )

IBSFetcherTest::IBSFetcherTest() : AbstractFetcherTest() {
}

void IBSFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void IBSFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QStringLiteral("Vino & cucina"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IBSFetcher(this));

  // the one we want should be in the first 5
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 5);

  QVERIFY(results.size() > 0);
  Tellico::Data::EntryPtr entry;  //  results can be randomly ordered, loop until we find the one we want
  foreach(Tellico::Data::EntryPtr testEntry, results) {
    if(testEntry->title().startsWith(QStringLiteral("Vino & cucina"), Qt::CaseInsensitive)) {
      entry = testEntry;
      break;
    }
  }
  QVERIFY(entry);

  compareEntry(entry);
}

void IBSFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("8804644060"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IBSFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  compareEntry(results.first());
}

void IBSFetcherTest::testTranslator() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("8842914975"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IBSFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.first();
  QCOMPARE(entry->field("title"), QString::fromUtf8("città sepolta"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("8842914975"));
  QCOMPARE(entry->field("pages"), QStringLiteral("542"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Avventura"));
  QCOMPARE(entry->field("language"), QStringLiteral("italiano"));
  QCOMPARE(entry->field("author"), QStringLiteral("James Rollins"));
  QCOMPARE(entry->field("translator"), QStringLiteral("Marco Zonetti"));
  QCOMPARE(entry->field("edition"), QStringLiteral("Nord"));
  QCOMPARE(entry->field("series"), QStringLiteral("Narrativa Nord"));
}

void IBSFetcherTest::testEditor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QStringLiteral("8873718735"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IBSFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.first();
  QCOMPARE(entry->field("title"), QString::fromUtf8("centuria bianca"));
  QCOMPARE(entry->field("editor"), QStringLiteral("I. Armaro"));
  QCOMPARE(entry->field("binding"), QStringLiteral("Paperback"));
}

void IBSFetcherTest::compareEntry(Tellico::Data::EntryPtr entry) {
  QVERIFY(entry->field("title").startsWith(QStringLiteral("Vino & cucina")));
  QCOMPARE(entry->field("isbn"), QStringLiteral("8804644060"));
  QCOMPARE(entry->field("author"), QStringLiteral("Antonella Clerici"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("2014"));
  QCOMPARE(entry->field("pages"), QStringLiteral("225"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Mondadori"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
