/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "winecomfetchertest.h"

#include "../fetch/winecomfetcher.h"
#include "../collections/winecollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( WineComFetcherTest )

WineComFetcherTest::WineComFetcherTest() : AbstractFetcherTest(), m_hasConfigFile(false)
    , m_config(QFINDTESTDATA("tellicotest_private.config"), KConfig::SimpleConfig) {
}

void WineComFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::WineCollection> registerWine(Tellico::Data::Collection::Wine, "wine");
  // since we use the importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/winecom2tellico.xsl"));
  Tellico::ImageFactory::init();

  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest_private.config"));
}

void WineComFetcherTest::testKeyword() {
  const QString groupName = QLatin1String("WineCom");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Wine.com settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Wine, Tellico::Fetch::Keyword,
                                       QLatin1String("1999 Eola Hills Pinot Noir"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::WineComFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("producer")), QLatin1String("Eola Hills"));
  QCOMPARE(entry->field(QLatin1String("appellation")), QLatin1String("Willamette Valley"));
  QCOMPARE(entry->field(QLatin1String("vintage")), QLatin1String("1999"));
  QCOMPARE(entry->field(QLatin1String("varietal")), QLatin1String("Pinot Noir"));
  QCOMPARE(entry->field(QLatin1String("type")), QLatin1String("Red Wine"));
  QVERIFY(!entry->field(QLatin1String("label")).isEmpty());
}
