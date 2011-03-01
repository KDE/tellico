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

#include "animenfofetchertest.h"
#include "animenfofetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/animenfofetcher.h"
#include "../entry.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"

#include <KConfigGroup>

QTEST_KDEMAIN( AnimenfoFetcherTest, GUI )

AnimenfoFetcherTest::AnimenfoFetcherTest() : m_loop(this) {
}

void AnimenfoFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
}

void AnimenfoFetcherTest::testMegami() {
  KConfig config(QString::fromLatin1(KDESRCDIR)  + "/tellicotest.config", KConfig::SimpleConfig);
  QString groupName = QLatin1String("AnimeNfo.com");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword, "Aa! Megami-sama!");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AnimeNfoFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  job->setMaximumResults(1);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = m_results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field("title"), QLatin1String("Aa! Megami-sama!"));
  QCOMPARE(entry->field("year"), QLatin1String("2011"));
  QCOMPARE(entry->field("episodes"), QLatin1String("2"));
  QCOMPARE(entry->field("studio"), QLatin1String("AIC (Anime International Company)"));
  QCOMPARE(entry->field("origtitle"), QString::fromUtf8("ああっ女神さまっ (2011)"));
  QVERIFY(entry->field("plot").startsWith(QLatin1String("Keiichi finds out")));
  QVERIFY(!entry->field("cover").isEmpty());
}

void AnimenfoFetcherTest::testHachimitsu() {
  KConfig config(QString::fromLatin1(KDESRCDIR)  + "/tellicotest.config", KConfig::SimpleConfig);
  QString groupName = QLatin1String("AnimeNfo.com");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword, "Hachimitsu to Clover");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AnimeNfoFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  job->setMaximumResults(1);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = m_results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field("title"), QLatin1String("Hachimitsu to Clover"));
  QCOMPARE(entry->field("year"), QLatin1String("2005"));
  QCOMPARE(entry->field("episodes"), QLatin1String("26"));
  QCOMPARE(entry->field("keyword"), QLatin1String("TV"));
  QCOMPARE(entry->field("genre"), QLatin1String("Comedy; Drama; Romance"));
  QCOMPARE(entry->field("studio"), QLatin1String("J.C.STAFF"));
  QCOMPARE(entry->field("origtitle"), QString::fromUtf8("ハチミツとクローバー"));
  QCOMPARE(entry->field("director"), QString::fromUtf8("Kasai Kenichi (カサヰ ケンイチ)"));
  QCOMPARE(entry->field("writer"), QString::fromUtf8("Kuroda Yosuke (黒田洋介)"));
  QCOMPARE(entry->field("alttitle"), QLatin1String("Honey and Clover"));
  QCOMPARE(entry->field("animenfo-rating"), QLatin1String("9"));
  QVERIFY(entry->field("plot").startsWith(QLatin1String("Takemoto, Mayama, and Morita are students")));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field("animenfo").isEmpty());
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QCOMPARE(castList.count(), 7);
  QCOMPARE(castList.at(0), QString::fromUtf8("Kudo Haruka (工藤晴香)::Hanamoto Hagumi"));
}

void AnimenfoFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
