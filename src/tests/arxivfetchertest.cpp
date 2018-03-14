/***************************************************************************
    Copyright (C) 2009-2011 Robby Stephenson <robby@periapsis.org>
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

#include "arxivfetchertest.h"

#include "../fetch/arxivfetcher.h"
#include "../entry.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( ArxivFetcherTest)

ArxivFetcherTest::ArxivFetcherTest() : AbstractFetcherTest() {
}

void ArxivFetcherTest::initTestCase() {
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/arxiv2tellico.xsl"));
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");

  m_fieldValues.insert(QStringLiteral("arxiv"), QStringLiteral("hep-lat/0110180"));
  m_fieldValues.insert(QStringLiteral("entry-type"), QStringLiteral("article"));
  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("Speeding up the Hybrid-Monte-Carlo algorithm for dynamical fermions"));
  m_fieldValues.insert(QStringLiteral("author"), QStringLiteral("M. Hasenbusch; K. Jansen"));
  m_fieldValues.insert(QStringLiteral("keyword"), QStringLiteral("High Energy Physics - Lattice"));
  m_fieldValues.insert(QStringLiteral("journal"), QStringLiteral("Nucl.Phys.Proc.Suppl. 106"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("2002"));
  m_fieldValues.insert(QStringLiteral("pages"), QStringLiteral("1076-1078"));
}

void ArxivFetcherTest::testArxivTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       QLatin1Char('"') + m_fieldValues.value(QStringLiteral("title")) + QLatin1Char('"'));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ArxivFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QEXPECT_FAIL("", "Exact title searches don't return expected results", Continue);
  QCOMPARE(results.size(), 1);
}

void ArxivFetcherTest::testArxivID() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::ArxivID,
                                       "arxiv:" + m_fieldValues.value(QStringLiteral("arxiv")));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ArxivFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QCOMPARE(entry->field(i.key()), i.value());
  }
}

void ArxivFetcherTest::testArxivIDVersioned() {
  QString arxivVersioned = m_fieldValues.value(QStringLiteral("arxiv")) + "v1";
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::ArxivID, arxivVersioned);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ArxivFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);
  // id has version since original search included it
  QCOMPARE(entry->field("arxiv"), arxivVersioned);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    if(i.key() == QLatin1String("arxiv")) continue;
    QCOMPARE(entry->field(i.key()), i.value());
  }
}
