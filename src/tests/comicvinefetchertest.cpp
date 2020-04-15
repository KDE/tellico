/***************************************************************************
    Copyright (C) 2019 Robby Stephenson <robby@periapsis.org>
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

#include "comicvinefetchertest.h"

#include "../fetch/comicvinefetcher.h"
#include "../collections/comicbookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( ComicVineFetcherTest )

ComicVineFetcherTest::ComicVineFetcherTest() : AbstractFetcherTest() {
}

void ComicVineFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::ComicBookCollection> registerCB(Tellico::Data::Collection::ComicBook, "comic");
  // since we use the importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/comicvine2tellico.xsl"));
  Tellico::ImageFactory::init();
}

void ComicVineFetcherTest::testKeyword() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("comicvine");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::Keyword,
                                       QStringLiteral("Avengers Endgame Prelude"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ComicVineFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("TPB"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2019"));
  QCOMPARE(entry->field(QStringLiteral("issue")), QStringLiteral("1"));
  QCOMPARE(entry->field(QStringLiteral("writer")), QStringLiteral("Will Corona Pilgrim"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Dono Almara; Paco Diaz Luque; Travis Lanham"));
  QCOMPARE(entry->field(QStringLiteral("series")), QStringLiteral("Marvel's Avengers: Endgame Prelude"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Marvel"));
  QCOMPARE(entry->field(QStringLiteral("comicvine")), QStringLiteral("https://comicvine.gamespot.com/marvels-avengers-endgame-prelude-1-tpb/4000-705478/"));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
