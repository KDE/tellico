/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "springerfetchertest.h"

#include "../fetch/springerfetcher.h"
#include "../entry.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( SpringerFetcherTest )

SpringerFetcherTest::SpringerFetcherTest() : AbstractFetcherTest() {
}

void SpringerFetcherTest::initTestCase() {
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/springer2tellico.xsl"));
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");

  m_fieldValues.insert(QStringLiteral("doi"), QStringLiteral("10.1007/BF02174211"));
//  m_fieldValues.insert(QStringLiteral("entry-type"), QStringLiteral("article"));
  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("The roughening transition of the three-dimensional Ising interface: A Monte Carlo study"));
  m_fieldValues.insert(QStringLiteral("author"), QStringLiteral("Hasenbusch, M.; Meyer, S.; Pütz, M."));
  m_fieldValues.insert(QStringLiteral("volume"), QStringLiteral("85"));
  m_fieldValues.insert(QStringLiteral("journal"), QStringLiteral("Journal of Statistical Physics"));
  m_fieldValues.insert(QStringLiteral("publisher"), QStringLiteral("Springer"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("1996"));
  m_fieldValues.insert(QStringLiteral("entry-type"), QStringLiteral("article"));
}

void SpringerFetcherTest::testTitle() {
  // title search requires Premium access
  return;
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       QStringLiteral("roughening transition of the three-dimensional Ising interface"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::SpringerFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QCOMPARE(entry->field(i.key()), i.value());
  }
  QVERIFY(entry->field(QStringLiteral("abstract")).contains(QStringLiteral("Kosterlitz-Thouless")));
}

void SpringerFetcherTest::testAuthor() {
  // author search requires Premium access
  return;
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Person,
                                       QStringLiteral("Albert Einstein"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::SpringerFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(results.size() > 1);
}

void SpringerFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Keyword,
                                       QStringLiteral("Hadron-hadron scattering"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::SpringerFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  // spring fetcher defaults to 10 at a time, expect 11 to check search continue
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 11);

  QCOMPARE(results.size(), 11);
}

void SpringerFetcherTest::testISBN() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::ISBN,
                                       QStringLiteral("978-3-7643-7436-5"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::SpringerFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  // there are several results for the same ISBN here
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
}

void SpringerFetcherTest::testDOI() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::DOI,
                                       m_fieldValues.value(QStringLiteral("doi")));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::SpringerFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QCOMPARE(entry->field(i.key()), i.value());
  }
  QVERIFY(entry->field(QStringLiteral("abstract")).contains(QStringLiteral("Kosterlitz-Thouless")));
}

void SpringerFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BibtexCollection(true));
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);
  entry->setField(QStringLiteral("doi"), m_fieldValues.value(QStringLiteral("doi")));

  Tellico::Fetch::SpringerFetcher fetcher(this);
  auto request = fetcher.updateRequest(entry);
  request.setCollectionType(coll->type());
  QCOMPARE(request.key(), Tellico::Fetch::DOI);
  QCOMPARE(request.value(), m_fieldValues.value(QStringLiteral("doi")));
}
