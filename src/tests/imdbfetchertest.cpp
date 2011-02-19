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

#include "imdbfetchertest.h"
#include "imdbfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/imdbfetcher.h"
#include "../entry.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"

#include <KConfigGroup>

QTEST_KDEMAIN( ImdbFetcherTest, GUI )

ImdbFetcherTest::ImdbFetcherTest() : m_loop(this) {
}

void ImdbFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
}

void ImdbFetcherTest::testSnowyRiver() {
  KConfig config(QString::fromLatin1(KDESRCDIR)  + "/tellicotest.config", KConfig::SimpleConfig);
  QString groupName = QLatin1String("IMDB");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "The Man From Snowy River");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(cg, cg.name());

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  job->setMaximumResults(1);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = m_results.at(0);

  QCOMPARE(entry->field("title"), QLatin1String("The Man from Snowy River"));
  QCOMPARE(entry->field("year"), QLatin1String("1982"));
  QCOMPARE(entry->field("genre"), QLatin1String("Adventure; Drama; Family; Romance; Western"));
  QCOMPARE(entry->field("nationality"), QLatin1String("Australia"));
  QCOMPARE(entry->field("studio"), QLatin1String("Cambridge Productions; Michael Edgley International; Snowy River Investment Pty. Ltd."));
  QCOMPARE(entry->field("running-time"), QLatin1String("102"));
  QCOMPARE(entry->field("audio-track"), QLatin1String("Dolby"));
  QCOMPARE(entry->field("aspect-ratio"), QLatin1String("2.35 : 1"));
  QCOMPARE(entry->field("color"), QLatin1String("Color"));
  QCOMPARE(entry->field("language"), QLatin1String("English"));
  QCOMPARE(entry->field("director"), QLatin1String("George Miller"));
  QCOMPARE(entry->field("writer"), QLatin1String("Cul Cullen; A.B. 'Banjo' Paterson"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QCOMPARE(castList.at(0), QLatin1String("Tom Burlinson::Jim Craig"));
  QCOMPARE(entry->field("imdb"), QLatin1String("http://akas.imdb.com/title/tt0084296/"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
}

void ImdbFetcherTest::testAsterix() {
  KConfig config(QString::fromLatin1(KDESRCDIR)  + "/tellicotest.config", KConfig::SimpleConfig);
  QString groupName = QLatin1String("IMDB");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "Astérix aux jeux olympiques");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));
  fetcher->readConfig(cg, cg.name());

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  job->setMaximumResults(1);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = m_results.at(0);

  QCOMPARE(entry->field("title"), QString::fromUtf8("Astérix aux jeux olympiques"));
  QCOMPARE(entry->field("director"), QString::fromUtf8("Thomas Langmann; Frédéric Forestier"));
  QCOMPARE(entry->field("writer"), QString::fromUtf8("René Goscinny; Albert Uderzo"));
  QStringList altTitleList = Tellico::FieldFormat::splitTable(entry->field("alttitle"));
  QVERIFY(altTitleList.size() > 1);
}

// https://bugs.kde.org/show_bug.cgi?id=249096
void ImdbFetcherTest::testBodyDouble() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "Body Double");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  job->setMaximumResults(1);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = m_results.at(0);

  QCOMPARE(entry->field("title"), QLatin1String("Body Double"));
  QCOMPARE(entry->field("director"), QLatin1String("Brian De Palma"));
  QCOMPARE(entry->field("writer"), QLatin1String("Brian De Palma; Robert J. Avrech"));
}

// https://bugs.kde.org/show_bug.cgi?id=249096
void ImdbFetcherTest::testMary() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "There's Something About Mary");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  job->setMaximumResults(1);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = m_results.at(0);

  QCOMPARE(entry->field("director"), QLatin1String("Peter Farrelly; Bobby Farrelly"));
  QCOMPARE(entry->field("writer"), QLatin1String("John J. Strauss; Ed Decter"));
}

// https://bugs.kde.org/show_bug.cgi?id=262036
void ImdbFetcherTest::testOkunen() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "46-okunen no koi");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::IMDBFetcher(this));

  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  job->setMaximumResults(1);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);
  Tellico::Data::EntryPtr entry = m_results.at(0);

  QCOMPARE(entry->field("year"), QLatin1String("2006"));
  QCOMPARE(entry->field("genre"), QLatin1String("Drama"));
  QCOMPARE(entry->field("director"), QLatin1String("Takashi Miike"));
  QCOMPARE(entry->field("writer"), QLatin1String("Ikki Kajiwara; Hisao Maki"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
}

void ImdbFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
