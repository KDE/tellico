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

#include "googlebookfetchertest.h"
#include "googlebookfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/googlebookfetcher.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN( GoogleBookFetcherTest, GUI )

GoogleBookFetcherTest::GoogleBookFetcherTest() : m_loop(this) {
}

void GoogleBookFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  // since we use the importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();
}

void GoogleBookFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Keyword,
                                       QLatin1String("Practical RDF"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoogleBookFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));
  job->setMaximumResults(1);

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Practical RDF"));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("0-596-00263-7"));
  QCOMPARE(entry->field(QLatin1String("author")), QLatin1String("Shelley Powers"));
  QCOMPARE(entry->field(QLatin1String("publisher")), QLatin1String("O'Reilly Media"));
  QCOMPARE(entry->field(QLatin1String("pages")), QLatin1String("331"));
  QCOMPARE(entry->field(QLatin1String("pub_year")), QLatin1String("2003"));
  QCOMPARE(entry->field(QLatin1String("keyword")), QLatin1String("Computers"));
  QVERIFY(!entry->field(QLatin1String("url")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
}

void GoogleBookFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
