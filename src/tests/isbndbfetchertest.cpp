/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "isbndbfetchertest.h"
#include "isbndbfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/isbndbfetcher.h"
#include "../entry.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN( ISBNdbFetcherTest, GUI )

ISBNdbFetcherTest::ISBNdbFetcherTest() : m_loop(this) {
}

void ISBNdbFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
}

void ISBNdbFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("0789312239"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);
//  QVERIFY(!m_results.isEmpty());

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")).toLower(), QLatin1String("this is venice"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Miroslav Sasek"));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("0-7893-1223-9"));
//  QCOMPARE(entry->field(QLatin1String("pub_year")), QLatin1String("2005"));
  QCOMPARE(entry->field(QLatin1String("publisher")), QLatin1String("Universe"));
  QCOMPARE(entry->field(QLatin1String("binding")), QLatin1String("Hardback"));
  QVERIFY(!entry->field(QLatin1String("comments")).isEmpty());
}

void ISBNdbFetcherTest::testIsbn13() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("9780596004361"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);
//  QVERIFY(!m_results.isEmpty());

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("C Pocket Reference"));
}

void ISBNdbFetcherTest::testMultipleIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("0789312239; 9780596000486"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ISBNdbFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QEXPECT_FAIL("", "ISBNdb fetcher does not yet support searching for multiple ISBNs", Continue);
  QCOMPARE(m_results.size(), 2);
}

void ISBNdbFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
