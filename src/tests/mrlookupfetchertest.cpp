/***************************************************************************
    Copyright (C) 2014 Robby Stephenson <robby@periapsis.org>
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

#include "mrlookupfetchertest.h"

#include "../fetch/mrlookupfetcher.h"
#include "../entry.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( MRLookupFetcherTest )

MRLookupFetcherTest::MRLookupFetcherTest() : AbstractFetcherTest() {
}

void MRLookupFetcherTest::initTestCase() {
  // since we use the bibtex importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../translators/bibtex-translation.xml"));

  m_fieldValues.insert(QStringLiteral("doi"), QStringLiteral("10.4169/amer.math.monthly.121.10.917"));
  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("An unnoticed consequence of Szegö's distribution theorem"));
  m_fieldValues.insert(QStringLiteral("author"), QStringLiteral("Trench, William F."));
  m_fieldValues.insert(QStringLiteral("volume"), QStringLiteral("121"));
  m_fieldValues.insert(QStringLiteral("journal"), QStringLiteral("American Mathematical Monthly"));
  m_fieldValues.insert(QStringLiteral("number"), QStringLiteral("10"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("2014"));
  m_fieldValues.insert(QStringLiteral("pages"), QStringLiteral("917–921"));
  m_fieldValues.insert(QStringLiteral("entry-type"), QStringLiteral("article"));
}

void MRLookupFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       QStringLiteral("An unnoticed consequence of Szego's distribution theorem"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MRLookupFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(!results.isEmpty());
  Tellico::Data::EntryPtr entry = results.at(0);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QCOMPARE(entry->field(i.key()), i.value());
  }
}

void MRLookupFetcherTest::testAuthor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Person,
                                       QStringLiteral("Trench, William"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MRLookupFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr test, results) {
    if(test->title().toLower() == m_fieldValues.value(QStringLiteral("title")).toLower()) {
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
    QCOMPARE(entry->field(i.key()), i.value());
  }
}
