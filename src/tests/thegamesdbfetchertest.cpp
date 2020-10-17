/***************************************************************************
    Copyright (C) 2012-2019 Robby Stephenson <robby@periapsis.org>
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

#include "thegamesdbfetchertest.h"

#include "../fetch/thegamesdbfetcher.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( TheGamesDBFetcherTest )

TheGamesDBFetcherTest::TheGamesDBFetcherTest() : AbstractFetcherTest() {
}

void TheGamesDBFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();

  QString configFile = QFINDTESTDATA("tellicotest_private.config");
  if(!configFile.isEmpty()) {
    m_config = KSharedConfig::openConfig(configFile, KConfig::SimpleConfig)->group(QStringLiteral("TGDB"));
  }

  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("GoldenEye 007"));
  m_fieldValues.insert(QStringLiteral("platform"), QStringLiteral("Nintendo 64"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("1997"));
  m_fieldValues.insert(QStringLiteral("certification"), QStringLiteral("Teen"));
  m_fieldValues.insert(QStringLiteral("genre"), QStringLiteral("Action; Adventure; Shooter; Stealth"));
  m_fieldValues.insert(QStringLiteral("publisher"), QStringLiteral("Nintendo"));
  m_fieldValues.insert(QStringLiteral("developer"), QStringLiteral("Rare, Ltd."));
  m_fieldValues.insert(QStringLiteral("num-player"), QStringLiteral("4"));

  // need to delete cached data
  QFile genreFile(Tellico::Fetch::TheGamesDBFetcher::dataFileName(Tellico::Fetch::TheGamesDBFetcher::Genre));
  if(genreFile.exists()) {
    genreFile.remove();
  }
  QFile publisherFile(Tellico::Fetch::TheGamesDBFetcher::dataFileName(Tellico::Fetch::TheGamesDBFetcher::Publisher));
  if(publisherFile.exists()) {
    publisherFile.remove();
  }
  QFile developerFile(Tellico::Fetch::TheGamesDBFetcher::dataFileName(Tellico::Fetch::TheGamesDBFetcher::Developer));
  if(developerFile.exists()) {
    developerFile.remove();
  }
}

void TheGamesDBFetcherTest::testTitle() {
  if(!m_config.isValid()) {
    QSKIP("This test requires a config file with TGDB settings.", SkipAll);
  }

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Title,
                                       QStringLiteral("Goldeneye"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheGamesDBFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
