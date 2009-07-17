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
#include "fetchertest.h"
#include "fetchertest.moc"

#include "../fetch/fetcherjob.h"
#include "../fetch/arxivfetcher.h"
#include "../entry.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN_CORE( FetcherTest )

FetcherTest::FetcherTest() : m_loop(this) {
}

void FetcherTest::initTestCase() {
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
}

void FetcherTest::testArxivFetcher() {
  // 5 is BibtexCollection
  Tellico::Fetch::FetchRequest request(5, Tellico::Fetch::ArxivID, "0901.3156v1");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ArxivFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  Tellico::Data::EntryList results = job->entries();
  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("arxiv"), QLatin1String("0901.3156v1"));
  QCOMPARE(entry->field("entry-type"), QLatin1String("article"));
  QCOMPARE(entry->field("title"), QLatin1String("Conservative solutions to the black hole information problem"));
  QCOMPARE(entry->field("author"), QLatin1String("Sabine Hossenfelder; Lee Smolin"));
  QCOMPARE(entry->field("keyword"), QLatin1String("General Relativity and Quantum Cosmology"));
}

void FetcherTest::slotResult(KJob*) {
  m_loop.quit();
}
