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

#include "qtest_kde.h"
#include "discogsfetchertest.h"
#include "discogsfetchertest.moc"

#include "../fetch/fetcherjob.h"
#include "../fetch/discogsfetcher.h"
#include "../collections/musiccollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN( DiscogsFetcherTest, GUI )

DiscogsFetcherTest::DiscogsFetcherTest() : m_loop(this) {
}

void DiscogsFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  // since we use the Discogs importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();
}

void DiscogsFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Title,
                                       QLatin1String("Fallen"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));
  job->setMaximumResults(1);

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  Tellico::Data::EntryPtr entry = m_results.at(0);
//  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Fallen"));
}

void DiscogsFetcherTest::testPeople() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Album, Tellico::Fetch::Person,
                                       QLatin1String("Evanescence"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DiscogsFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));
  job->setMaximumResults(1);

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QCOMPARE(entry->field(QLatin1String("artist")), QLatin1String("Evanescence"));
  QVERIFY(!entry->field(QLatin1String("title")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("label")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("year")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("track")).isEmpty());
}

void DiscogsFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
