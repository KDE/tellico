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

#include "filmasterfetchertest.h"
#include "filmasterfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/filmasterfetcher.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN( FilmasterFetcherTest, GUI )

FilmasterFetcherTest::FilmasterFetcherTest() : m_loop(this) {
}

void FilmasterFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QLatin1String("title"), QLatin1String("The Man from Snowy River"));
  m_fieldValues.insert(QLatin1String("year"), QLatin1String("1982"));
  m_fieldValues.insert(QLatin1String("genre"), QLatin1String("drama; family; romance; western"));
  m_fieldValues.insert(QLatin1String("director"), QLatin1String("George Miller"));
//  m_fieldValues.insert(QLatin1String("nationality"), QLatin1String("Australia"));
}

void FilmasterFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QLatin1String("Man From Snowy River"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FilmasterFetcher(this));

  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));
  job->setMaximumResults(2);

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 2);

  Tellico::Data::EntryPtr entry = m_results.at(1);
  QVERIFY(entry);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QCOMPARE(castList.at(0), QLatin1String("Tom Burlinson::Jim Craig"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
}

void FilmasterFetcherTest::testPerson() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Person,
                                       QLatin1String("Tom Burlinson"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FilmasterFetcher(this));

  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QVERIFY(m_results.size() > 0);
  Tellico::Data::EntryPtr entry;  //  results can be randomly ordered, loop until wee find the one we want
  for(int i = 0; i < m_results.size(); ++i) {
    Tellico::Data::EntryPtr test = m_results.at(i);
    if(test->field(QLatin1String("title")).toLower() == QLatin1String("the man from snowy river")) {
      entry = test;
      break;
    } else {
      qDebug() << "skipping" << test->title();
    }
  }
  QVERIFY(entry);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QCOMPARE(castList.at(0), QLatin1String("Tom Burlinson::Jim Craig"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
}

void FilmasterFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword,
                                       QLatin1String("Man From Snowy River"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FilmasterFetcher(this));

  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));
  job->setMaximumResults(2);

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 2);

  Tellico::Data::EntryPtr entry = m_results.at(1);
  QVERIFY(entry);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QCOMPARE(castList.at(0), QLatin1String("Tom Burlinson::Jim Craig"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
}

void FilmasterFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
