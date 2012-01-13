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

#include "bibliosharefetchertest.h"
#include "bibliosharefetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/bibliosharefetcher.h"
#include "../entry.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"

#include <KStandardDirs>
#include <KLocale>

QTEST_KDEMAIN( BiblioShareFetcherTest, GUI )

BiblioShareFetcherTest::BiblioShareFetcherTest() : AbstractFetcherTest() {
}

void BiblioShareFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  // since we use the importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
}

void BiblioShareFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("0670069035"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BiblioShareFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("The Girl Who Kicked The Hornets Nest"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Stieg Larsson"));
  QCOMPARE(entry->field(QLatin1String("binding")), QLatin1String("Hardback"));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("0-670-06903-5"));
  QCOMPARE(entry->field(QLatin1String("pub_year")), QLatin1String("2010"));
  QCOMPARE(entry->field(QLatin1String("publisher")), QLatin1String("Penguin Group Canada"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}

void BiblioShareFetcherTest::testIsbn13() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("9780670069033"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BiblioShareFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("The Girl Who Kicked The Hornets Nest"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Stieg Larsson"));
  QCOMPARE(entry->field(QLatin1String("binding")), QLatin1String("Hardback"));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("0-670-06903-5"));
  QCOMPARE(entry->field(QLatin1String("pub_year")), QLatin1String("2010"));
  QCOMPARE(entry->field(QLatin1String("publisher")), QLatin1String("Penguin Group Canada"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}
