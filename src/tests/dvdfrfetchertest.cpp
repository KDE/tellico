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

#include "dvdfrfetchertest.h"
#include "dvdfrfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/dvdfrfetcher.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN( DVDFrFetcherTest, GUI )

DVDFrFetcherTest::DVDFrFetcherTest() : m_loop(this) {
}

void DVDFrFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  // since we use the importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QLatin1String("title"), QLatin1String("Le Pacte des loups"));
  m_fieldValues.insert(QLatin1String("studio"), QLatin1String("StudioCanal"));
  m_fieldValues.insert(QLatin1String("year"), QLatin1String("2001"));
//  m_fieldValues.insert(QLatin1String("medium"), QLatin1String("DVD"));
  m_fieldValues.insert(QLatin1String("format"), QLatin1String("PAL"));
  m_fieldValues.insert(QLatin1String("aspect-ratio"), QLatin1String("2.35"));
  m_fieldValues.insert(QLatin1String("writer"), QString::fromUtf8("Stéphane Cabel; Christophe Gans"));
  m_fieldValues.insert(QLatin1String("director"), QLatin1String("Christophe Gans"));
  m_fieldValues.insert(QLatin1String("genre"), QLatin1String("Aventure; Fantastique"));
  m_fieldValues.insert(QLatin1String("widescreen"), QLatin1String("true"));
}

void DVDFrFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QLatin1String("Le Pacte des loups"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DVDFrFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));
  job->setMaximumResults(1);

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QLatin1String("cast")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("comments")).isEmpty());
}

void DVDFrFetcherTest::testUPC() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::UPC,
                                       QLatin1String("3259119636120"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DVDFrFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QLatin1String("cast")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("comments")).isEmpty());
}

void DVDFrFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
