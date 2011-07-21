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

#include "googlescholarfetchertest.h"
#include "googlescholarfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/googlescholarfetcher.h"
#include "../entry.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../translators/bibtexhandler.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN( GoogleScholarFetcherTest, GUI )

GoogleScholarFetcherTest::GoogleScholarFetcherTest() : m_loop(this) {
}

void GoogleScholarFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
  // since we use the bibtex importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../translators/");

  m_fieldValues.insert(QLatin1String("entry-type"), QLatin1String("article"));
  m_fieldValues.insert(QLatin1String("title"), QLatin1String("Speeding up the Hybrid Monte Carlo algorithm for dynamical fermions"));
  m_fieldValues.insert(QLatin1String("author"), QLatin1String("Hasenbusch, M."));
  m_fieldValues.insert(QLatin1String("publisher"), QLatin1String("Elsevier"));
  m_fieldValues.insert(QLatin1String("journal"), QLatin1String("physics letters b"));
  m_fieldValues.insert(QLatin1String("volume"), QLatin1String("519"));
  m_fieldValues.insert(QLatin1String("year"), QLatin1String("2001"));
  m_fieldValues.insert(QLatin1String("pages"), QString::fromUtf8("177–182"));
  m_fieldValues.insert(QLatin1String("bibtex-key"), QLatin1String("hasenbusch2001speeding"));
}

void GoogleScholarFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       QLatin1Char('"') + m_fieldValues.value("title") + QLatin1Char('"'));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoogleScholarFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

//  QCOMPARE(m_results.size(), 1);
  QVERIFY(!m_results.isEmpty());

  if(!m_results.isEmpty()) {
    Tellico::Data::EntryPtr entry = m_results.at(0);

    QHashIterator<QString, QString> i(m_fieldValues);
    while(i.hasNext()) {
      i.next();
      QString result = entry->field(i.key()).toLower();
      Tellico::BibtexHandler::cleanText(result);
      QCOMPARE(result, i.value().toLower());
    }
  }
}

void GoogleScholarFetcherTest::testAuthor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Person,
                                       QLatin1Char('"') + m_fieldValues.value("author") + QLatin1Char('"'));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoogleScholarFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

//  QCOMPARE(m_results.size(), 1);
  QVERIFY(!m_results.isEmpty());

  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr test, m_results) {
    if(test->title().toLower() == m_fieldValues.value(QLatin1String("title")).toLower()) {
      entry = test;
      break;
    } else {
      qDebug() << "Skipping" << test->title();
    }
  }
  QVERIFY(entry);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    Tellico::BibtexHandler::cleanText(result);
    QCOMPARE(result, i.value().toLower());
  }
}

void GoogleScholarFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
