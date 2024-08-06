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

#undef QT_NO_CAST_FROM_ASCII

#include "rpggeekfetchertest.h"

#include "../fetch/rpggeekfetcher.h"
#include "../collection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( RPGGeekFetcherTest )

RPGGeekFetcherTest::RPGGeekFetcherTest() : AbstractFetcherTest() {
}

void RPGGeekFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::Collection> registerColl(Tellico::Data::Collection::Base, "entry");
  Tellico::ImageFactory::init();
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/boardgamegeek2tellico.xsl"));
}

void RPGGeekFetcherTest::testKeyword() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("rpggeek"));
  cg.writeEntry("Custom Fields", QStringLiteral("genre,year,publisher,artist,designer,producer,mechanism,description,rpggeek-link"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Base, Tellico::Fetch::Keyword,
                                       QStringLiteral("Winds of the North"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::RPGGeekFetcher(this));
  QVERIFY(fetcher->canSearch(request.key()));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Base);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Winds of the North"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("(Self-Published)"));
  QCOMPARE(entry->field(QStringLiteral("designer")), QStringLiteral("Thomas King"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Nils Bergslien"));
  QCOMPARE(entry->field(QStringLiteral("producer")), QStringLiteral("Thomas King"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Culture; History; History (Medieval); History (Vikings); Mythology / Folklore"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("0"));
  auto genres = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("genre")));
  QVERIFY(genres.count() > 2);
  QVERIFY(genres.contains(QStringLiteral("Culture")));
  auto mechs = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("mechanism")));
  QVERIFY(mechs.count() > 2);
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("description")).contains(QLatin1String("#10;")));
  QCOMPARE(entry->field(QStringLiteral("rpggeek-link")), QStringLiteral("https://rpggeek.com/rpgitem/338762"));
}

void RPGGeekFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true));
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("bggid"), QStringLiteral("bggid")));
  coll->addField(field);
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);
  entry->setField(QStringLiteral("bggid"), QStringLiteral("338762"));

  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("rpggeek"));
  cg.writeEntry("Custom Fields", QStringLiteral("genre,year,publisher,artist,designer,producer,mechanism,description,rpggeek-link"));

  Tellico::Fetch::RPGGeekFetcher fetcher(this);
  auto request = fetcher.updateRequest(entry);
  request.setCollectionType(coll->type());
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QCOMPARE(request.value(), QStringLiteral("338762"));
  fetcher.readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(&fetcher, request, 1);
  QCOMPARE(results.size(), 1);

  entry = results.at(0);
  QCOMPARE(entry->collection()->type(), Tellico::Data::Collection::Base);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Winds of the North"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("(Self-Published)"));
  QCOMPARE(entry->field(QStringLiteral("designer")), QStringLiteral("Thomas King"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Nils Bergslien"));
  QCOMPARE(entry->field(QStringLiteral("producer")), QStringLiteral("Thomas King"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Culture; History; History (Medieval); History (Vikings); Mythology / Folklore"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("0"));
  auto genres = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("genre")));
  QVERIFY(genres.count() > 2);
  QVERIFY(genres.contains(QStringLiteral("Culture")));
  auto mechs = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("mechanism")));
  QVERIFY(mechs.count() > 2);
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QCOMPARE(entry->field(QStringLiteral("rpggeek-link")), QStringLiteral("https://rpggeek.com/rpgitem/338762"));
}
