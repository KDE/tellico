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

#include "boardgamegeekfetchertest.h"
#include "boardgamegeekfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/execexternalfetcher.h"
#include "../collections/boardgamecollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KStandardDirs>
#include <KConfigGroup>

QTEST_KDEMAIN( BoardGameGeekFetcherTest, GUI )

BoardGameGeekFetcherTest::BoardGameGeekFetcherTest() : m_loop(this) {
}

void BoardGameGeekFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BoardGameCollection> registerBoard(Tellico::Data::Collection::BoardGame, "boardgame");
  Tellico::ImageFactory::init();
}

void BoardGameGeekFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::BoardGame, Tellico::Fetch::Title,
                                       QLatin1String("settlers of catan"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/boardgamegeek.rb.spec", KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QLatin1String("<default>"));
  cg.writeEntry("ExecPath", QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/boardgamegeek.rb");
  // don't sync() and save the new path
  cg.markAsClean();
  fetcher->readConfig(cg, cg.name());

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));
  job->setMaximumResults(1);

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 1);

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("The Settlers of Catan"));
  QCOMPARE(entry->field(QLatin1String("designer")), QLatin1String("Klaus Teuber"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field(QLatin1String("publisher"))).at(0), QLatin1String("999 Games"));
  QCOMPARE(entry->field(QLatin1String("year")), QLatin1String("1995"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field(QLatin1String("genre"))).at(0), QLatin1String("City Building"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field(QLatin1String("mechanism"))).at(0), QLatin1String("Dice Rolling"));
  QCOMPARE(entry->field(QLatin1String("num-player")), QLatin1String("3; 4"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("description")).isEmpty());
}

void BoardGameGeekFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
