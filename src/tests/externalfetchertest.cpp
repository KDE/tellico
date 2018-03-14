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

#include "externalfetchertest.h"

#include "../fetch/execexternalfetcher.h"
#include "../entry.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../utils/datafileregistry.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( ExternalFetcherTest )

ExternalFetcherTest::ExternalFetcherTest() : AbstractFetcherTest() {
}

void ExternalFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/mods2tellico.xsl"));
}

void ExternalFetcherTest::testMods() {
  // fake the fetcher by 'cat'ting the MODS file
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title,
                                       QFINDTESTDATA("data/example_mods.xml"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QFINDTESTDATA("data/cat_mods.spec"), KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QStringLiteral("<default>"));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Sound and fury"));
  QCOMPARE(entry->field("author"), QStringLiteral("Alterman, Eric"));
  QCOMPARE(entry->field("genre"), QStringLiteral("bibliography"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("1999"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("0-8014-8639-4"));
  QCOMPARE(entry->field("lccn"), QStringLiteral("99042030"));
}
