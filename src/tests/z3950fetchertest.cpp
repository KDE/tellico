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

#include "z3950fetchertest.h"
#include "z3950fetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/z3950fetcher.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN( Z3950FetcherTest, GUI )

Z3950FetcherTest::Z3950FetcherTest() : m_loop(this) {
}

void Z3950FetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBibtex(Tellico::Data::Collection::Book, "book");
  // since we use the MODS importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  // also use the z3950-servers.cfg file
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../fetch/");
}

void Z3950FetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QLatin1String("Foundations of Qt Development"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::Z3950Fetcher(this, QLatin1String("loc")));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Foundations of Qt development"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Thelin, Johan."));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("1-59059-831-8"));
}

void Z3950FetcherTest::testIsbn() {
  // also testing multiple values
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       QLatin1String("978-1-59059-831-3; 0-201-88954-4"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::Z3950Fetcher(this, QLatin1String("loc")));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 2);

  Tellico::Data::EntryPtr entry = m_results.at(1);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Foundations of Qt development"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Thelin, Johan."));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("1-59059-831-8"));
}

void Z3950FetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
