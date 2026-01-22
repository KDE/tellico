/***************************************************************************
    Copyright (C) 2026 Robby Stephenson <robby@periapsis.org>
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

#include "doifetchertest.h"

#include "../fetch/doifetcher.h"
#include "../entry.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( DOIFetcherTest )

DOIFetcherTest::DOIFetcherTest() : AbstractFetcherTest() {
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/unixref2tellico.xsl"));
}

void DOIFetcherTest::initTestCase() {
  // since we use the bibtex importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../translators/bibtex-translation.xml"));
}

void DOIFetcherTest::testDOI() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex,
                                       Tellico::Fetch::DOI,
                                       QStringLiteral("10.2514/1.G000894"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DOIFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field(QStringLiteral("doi")), QStringLiteral("10.2514/1.g000894")); // lower-case
  QCOMPARE(entry->field(QStringLiteral("entry-type")), QStringLiteral("article"));
  QCOMPARE(entry->field(QStringLiteral("bibtex-key")), QStringLiteral("Delabie_2015"));
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Robustness and Efficiency Improvements for Star Tracker Attitude Estimation"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Delabie, Tjorven; Schutter, Joris De; Vandenbussche, Bart"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("2108\u20132121"));
  QCOMPARE(entry->field(QStringLiteral("journal")), QStringLiteral("Journal of Guidance, Control, and Dynamics"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("American Institute of Aeronautics and Astronautics (AIAA)"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2015"));
  QCOMPARE(entry->field(QStringLiteral("month")), QStringLiteral("nov")); // macro
  QCOMPARE(entry->field(QStringLiteral("volume")), QStringLiteral("38"));
  QCOMPARE(entry->field(QStringLiteral("number")), QStringLiteral("11"));
  QCOMPARE(entry->field(QStringLiteral("issn")), QStringLiteral("1533-3884"));
  QCOMPARE(entry->field(QStringLiteral("url")), QStringLiteral("http://dx.doi.org/10.2514/1.G000894")); // upper-case
}
