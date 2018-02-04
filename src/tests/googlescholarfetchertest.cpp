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

#include "googlescholarfetchertest.h"

#include "../fetch/googlescholarfetcher.h"
#include "../entry.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../utils/bibtexhandler.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( GoogleScholarFetcherTest )

GoogleScholarFetcherTest::GoogleScholarFetcherTest() : AbstractFetcherTest() {
}

void GoogleScholarFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
  // since we use the bibtex importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../translators/bibtex-translation.xml"));

  m_fieldValues.insert(QStringLiteral("entry-type"), QStringLiteral("article"));
  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("Speeding up the Hybrid Monte Carlo algorithm for dynamical fermions"));
  m_fieldValues.insert(QStringLiteral("author"), QStringLiteral("Hasenbusch, Martin"));
  m_fieldValues.insert(QStringLiteral("publisher"), QStringLiteral("Elsevier"));
  m_fieldValues.insert(QStringLiteral("journal"), QStringLiteral("physics letters b"));
  m_fieldValues.insert(QStringLiteral("volume"), QStringLiteral("519"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("2001"));
  m_fieldValues.insert(QStringLiteral("pages"), QStringLiteral("177–182"));
  m_fieldValues.insert(QStringLiteral("bibtex-key"), QStringLiteral("hasenbusch2001speeding"));
}

void GoogleScholarFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       m_fieldValues.value(QStringLiteral("title")));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoogleScholarFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    Tellico::BibtexHandler::cleanText(result);
    QCOMPARE(result, i.value().toLower());
  }
}

void GoogleScholarFetcherTest::testAuthor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Person,
                                       m_fieldValues.value(QStringLiteral("author")));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GoogleScholarFetcher(this));

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
    QString result = entry->field(i.key()).toLower();
    Tellico::BibtexHandler::cleanText(result);
    QCOMPARE(result, i.value().toLower());
  }
}
