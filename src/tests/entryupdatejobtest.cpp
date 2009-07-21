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
#include "entryupdatejobtest.h"
#include "entryupdatejobtest.moc"

#include "../entryupdatejob.h"
#include "../fetch/arxivfetcher.h"
#include "../entry.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN_CORE( EntryUpdateJobTest )

namespace {
  // 5 is BibtexCollection
  static const int COLLECTION_TYPE = 5;
}

EntryUpdateJobTest::EntryUpdateJobTest() : m_loop(this) {
}

void EntryUpdateJobTest::initTestCase() {
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
}

void EntryUpdateJobTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BibtexCollection(true));
  Tellico::Data::FieldPtr field(new Tellico::Data::Field("arxiv", "Arxiv ID"));
  coll->addField(field);
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField("arxiv", "hep-lat/0110180");
  coll->addEntries(entry);

  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ArxivFetcher(this));

  // don't use 'this' as job parent, it crashes
  Tellico::EntryUpdateJob* job = new Tellico::EntryUpdateJob(0, entry, fetcher);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(entry->field("title"), QLatin1String("Speeding up the Hybrid-Monte-Carlo algorithm for dynamical fermions"));
}

void EntryUpdateJobTest::slotResult(KJob*) {
  m_loop.quit();
}
