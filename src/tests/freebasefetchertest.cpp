/***************************************************************************
    Copyright (C) 2010 Robby Stephenson <robby@periapsis.org>
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

#include "qtest_kde.h"
#include "freebasefetchertest.h"
#include "freebasefetchertest.moc"

#include "../fetch/fetcherjob.h"
#include "../fetch/freebasefetcher.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

QTEST_KDEMAIN( FreebaseFetcherTest, GUI )

FreebaseFetcherTest::FreebaseFetcherTest() : m_loop(this) {
}

void FreebaseFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerGame(Tellico::Data::Collection::Book, "book");
  Tellico::ImageFactory::init();
}

void FreebaseFetcherTest::testISBN() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("9780321113580"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FreebaseFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));
  job->setMaximumResults(1);

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")).toLower(), QLatin1String("c++ coding standards: 101 rules, guidelines, and best practices"));
  QCOMPARE(entry->field(QLatin1String("publisher")), QLatin1String("Addison-Wesley"));
  QCOMPARE(entry->field(QLatin1String("pub_year")), QLatin1String("2004"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Herb Sutter"));
  QCOMPARE(entry->field(QLatin1String("series")), QLatin1String("C++ In-Depth Series"));
  QCOMPARE(entry->field(QLatin1String("lccn")), QLatin1String("2004022605"));
  QCOMPARE(entry->field(QLatin1String("binding")), QLatin1String("Trade Paperback"));
  QCOMPARE(entry->field(QLatin1String("pages")), QLatin1String("220"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}

void FreebaseFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
