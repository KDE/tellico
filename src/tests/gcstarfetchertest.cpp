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

#include "qtest_kde.h"
#include "gcstarfetchertest.h"
#include "gcstarfetchertest.moc"

#include "../fetch/fetcherjob.h"
#include "../fetch/gcstarpluginfetcher.h"
#include "../entry.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"

#include <KConfigGroup>
#include <KStandardDirs>

QTEST_KDEMAIN( GCstarFetcherTest, GUI )

GCstarFetcherTest::GCstarFetcherTest() : m_loop(this) {
}

void GCstarFetcherTest::initTestCase() {
  const QString gcstar = KStandardDirs::findExe(QLatin1String("gcstar"));
  if(gcstar.isEmpty()) {
    QSKIP("This test requires gcstar", SkipAll);
  }

  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");

  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
}

void GCstarFetcherTest::testSnowyRiver() {
  KConfig config(QString::fromLatin1(KDESRCDIR)  + "/tellicotest.config", KConfig::SimpleConfig);
  QString groupName = QLatin1String("GCstar Video");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, "The Man From Snowy River");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GCstarPluginFetcher(this));
  fetcher->readConfig(cg, cg.name());

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
}

void GCstarFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
