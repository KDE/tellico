/***************************************************************************
    Copyright (C) 2015 Robby Stephenson <robby@periapsis.org>
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

#include "crossreffetchertest.h"

#include "../fetch/crossreffetcher.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../utils/datafileregistry.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( CrossRefFetcherTest )

CrossRefFetcherTest::CrossRefFetcherTest() : AbstractFetcherTest() {
}

void CrossRefFetcherTest::initTestCase() {
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/unixref2tellico.xsl"));
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");

  m_fieldValues.insert(QStringLiteral("doi"), QStringLiteral("10.2514/1.G000894"));
  m_fieldValues.insert(QStringLiteral("entry-type"), QStringLiteral("article"));
  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("Robustness and Efficiency Improvements for Star Tracker Attitude Estimation"));
  m_fieldValues.insert(QStringLiteral("author"), QStringLiteral("Tjorven Delabie; Joris De Schutter; Bart Vandenbussche"));
  m_fieldValues.insert(QStringLiteral("pages"), QStringLiteral("2108-2121"));
  m_fieldValues.insert(QStringLiteral("journal"), QStringLiteral("Journal of Guidance, Control, and Dynamics"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("2015"));
  m_fieldValues.insert(QStringLiteral("month"), QStringLiteral("11"));
}

void CrossRefFetcherTest::testDOI() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("crossref");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::DOI,
                                       m_fieldValues.value(QStringLiteral("doi")));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::CrossRefFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QCOMPARE(entry->field(i.key()), i.value());
  }
}
