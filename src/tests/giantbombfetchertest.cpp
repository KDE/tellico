/***************************************************************************
    Copyright (C) 2010-2011 Robby Stephenson <robby@periapsis.org>
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

#include "giantbombfetchertest.h"

#include "../fetch/giantbombfetcher.h"
#include "../collections/gamecollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( GiantBombFetcherTest )

GiantBombFetcherTest::GiantBombFetcherTest() : AbstractFetcherTest() {
}

void GiantBombFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::GameCollection> registerGame(Tellico::Data::Collection::Game, "game");
  // since we use the importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/giantbomb2tellico.xsl"));
  Tellico::ImageFactory::init();
}

void GiantBombFetcherTest::testKeyword() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("giantbomb");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Keyword,
                                       QStringLiteral("Halo 3: ODST"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GiantBombFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Halo 3: ODST"));
  QCOMPARE(entry->field(QStringLiteral("developer")), QStringLiteral("Bungie"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2009"));
  QCOMPARE(entry->field(QStringLiteral("platform")), QStringLiteral("Xbox 360"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Action; First-Person Shooter"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Microsoft Studios"));
  QCOMPARE(entry->field(QStringLiteral("certification")), QStringLiteral("Mature"));
  QCOMPARE(entry->field(QStringLiteral("giantbomb")), QStringLiteral("https://www.giantbomb.com/halo-3-odst/3030-24035/"));
  QCOMPARE(entry->field(QStringLiteral("pegi")), QStringLiteral("PEGI 16"));
}
