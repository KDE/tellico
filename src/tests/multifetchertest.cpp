/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "multifetchertest.h"

#include "../fetch/multifetcher.h"
#include "../fetch/execexternalfetcher.h"
#include "../fetch/fetchmanager.h"
#include "../fetch/messagelogger.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../utils/datafileregistry.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( MultiFetcherTest )

MultiFetcherTest::MultiFetcherTest() : AbstractFetcherTest() {
}

void MultiFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/mods2tellico.xsl"));
}

void MultiFetcherTest::testEmpty() {
  auto config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("multi"));
  config.writeEntry("CollectionType", int(Tellico::Data::Collection::Book));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::Title, QString());
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MultiFetcher(this));
  fetcher->readConfig(config);

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);
  QVERIFY(results.isEmpty());
}

void MultiFetcherTest::testIsbn() {
  // just going to chain two trivial data sources together to test multifetcher
  Tellico::Fetch::Fetcher::Ptr modsFetcher1(new Tellico::Fetch::ExecExternalFetcher(this));
  Tellico::Fetch::Fetcher::Ptr modsFetcher2(new Tellico::Fetch::ExecExternalFetcher(this));

  KSharedConfig::Ptr catConfig = KSharedConfig::openConfig(QFINDTESTDATA("data/cat_mods.spec"), KConfig::SimpleConfig);
  KConfigGroup catConfigGroup = catConfig->group(QStringLiteral("<default>"));
  catConfigGroup.writeEntry("ExecPath", QFINDTESTDATA("data/cat_mods.sh")); // update command path to local script
  catConfigGroup.markAsClean(); // don't edit the file on sync()
  modsFetcher1->readConfig(catConfigGroup);
  modsFetcher1->setMessageHandler(new Tellico::Fetch::MessageLogger);
  modsFetcher2->readConfig(catConfigGroup);
  modsFetcher2->setMessageHandler(new Tellico::Fetch::MessageLogger);

  auto fetchManager = Tellico::Fetch::Manager::self();
  fetchManager->addFetcher(modsFetcher1);
  fetchManager->addFetcher(modsFetcher2);

  QStringList uuids;
  uuids << modsFetcher1->uuid() << modsFetcher2->uuid();

  auto multiConfig = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("multi"));
  multiConfig.writeEntry("Sources", uuids);
  multiConfig.writeEntry("CollectionType", int(Tellico::Data::Collection::Book));

  Tellico::Fetch::FetchRequest isbnRequest(Tellico::Data::Collection::Book,
                                           Tellico::Fetch::ISBN,
                                           QStringLiteral("0801486394"));
  Tellico::Fetch::Fetcher::Ptr multiFetcher(new Tellico::Fetch::MultiFetcher(this));
  multiFetcher->readConfig(multiConfig);
  multiFetcher->setMessageHandler(new Tellico::Fetch::MessageLogger);

  Tellico::Data::EntryList results = DO_FETCH(multiFetcher, isbnRequest);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Sound and fury"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-8014-8639-4"));
}
