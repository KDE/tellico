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
#include "../collections/gamecollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( TheGamesDBFetcherTest )

TheGamesDBFetcherTest::TheGamesDBFetcherTest() : AbstractFetcherTest() {
}

void TheGamesDBFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("GoldenEye 007"));
  m_fieldValues.insert(QStringLiteral("platform"), QStringLiteral("Nintendo 64"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("1997"));
  m_fieldValues.insert(QStringLiteral("certification"), QStringLiteral("Teen"));
  m_fieldValues.insert(QStringLiteral("genre"), QStringLiteral("Action; Shooter; Stealth"));
  m_fieldValues.insert(QStringLiteral("publisher"), QStringLiteral("Nintendo of America, inc."));
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
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("TGDB"));
  cg.writeEntry("Custom Fields", QStringLiteral("num-player,screenshot"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Title,
                                       QStringLiteral("Goldeneye 007"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheGamesDBFetcher(this));
  fetcher->readConfig(cg);
  QVERIFY(fetcher->canSearch(request.key()));

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
  QVERIFY(!entry->field(QStringLiteral("screenshot")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("screenshot")).contains(QLatin1Char('/')));
}

void TheGamesDBFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::GameCollection(true));
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);
  entry->setField(QStringLiteral("title"), QStringLiteral("GoldenEye 007"));
  entry->setField(QStringLiteral("platform"), QStringLiteral("Nintendo 64"));

  Tellico::Fetch::TheGamesDBFetcher fetcher(this);
  qApp->processEvents(); // allow time for timer to load cached data
  auto request = fetcher.updateRequest(entry);
  QCOMPARE(request.key(), Tellico::Fetch::Title);
  QCOMPARE(request.value(), QStringLiteral("GoldenEye 007"));
  QCOMPARE(request.data(), QLatin1String("3"));

  request.setCollectionType(coll->type());
  Tellico::Data::EntryList results = DO_FETCH1(&fetcher, request, 1);
  QCOMPARE(results.size(), 1);
}
