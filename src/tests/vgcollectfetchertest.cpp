/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#include "vgcollectfetchertest.h"

#include "../fetch/vgcollectfetcher.h"
#include "../collections/gamecollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( VGCollectFetcherTest )

VGCollectFetcherTest::VGCollectFetcherTest() : AbstractFetcherTest() {
}

void VGCollectFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void VGCollectFetcherTest::testKeyword() {
  auto config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("vgcollect"));
  config.writeEntry("Custom Fields", QStringLiteral("vgcollect,pegi,barcode"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Keyword,
                                       QStringLiteral("Sunset Overdrive - Day One Edition"));
  QScopedPointer<Tellico::Fetch::Fetcher> fetcher(new Tellico::Fetch::VGCollectFetcher(this));
  fetcher->readConfig(config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher.data(), request, 5);

  QVERIFY(!results.isEmpty());
  // want the Wii version
  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr tryEntry, results) {
    if(tryEntry->field(QStringLiteral("platform")) == QLatin1String("Xbox One") &&
       tryEntry->field(QStringLiteral("publisher")) == QLatin1String("Microsoft")) {
      entry = tryEntry;
      break;
    }
  }

  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Sunset Overdrive - Day One Edition"));
  QCOMPARE(entry->field("year"), QStringLiteral("2014"));
  QCOMPARE(entry->field("platform"), QStringLiteral("Xbox One"));
  QCOMPARE(entry->field("certification"), QStringLiteral("Mature"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Action"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Microsoft"));
  QCOMPARE(entry->field("developer"), QStringLiteral("Insomniac Games"));
  QCOMPARE(entry->field("barcode"), QStringLiteral("885370848847"));
  QCOMPARE(entry->field("vgcollect"), QStringLiteral("https://vgcollect.com/item/68543"));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
