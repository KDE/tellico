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

#include "thetvdbfetchertest.h"

#include "../fetch/thetvdbfetcher.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( TheTVDBFetcherTest )

TheTVDBFetcherTest::TheTVDBFetcherTest() : AbstractFetcherTest() {
}

void TheTVDBFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest_private.config"));
  if(m_hasConfigFile) {
    m_config = KSharedConfig::openConfig(QFINDTESTDATA("tellicotest_private.config"), KConfig::SimpleConfig);
  }
}

void TheTVDBFetcherTest::testTitle() {
  const QString groupName = QStringLiteral("TheTVDB");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with TheTVDB settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("Firefly"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheTVDBFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);
  fetcher->saveConfig();

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Firefly"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2002"));
  QCOMPARE(entry->field(QStringLiteral("network")), QStringLiteral("FOX"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("English"));
  // 2021-10-04 - content rating didn't seem to be returned in the data
//  QCOMPARE(entry->field(QStringLiteral("certification")), QStringLiteral("TV-14"));
  QCOMPARE(entry->field(QStringLiteral("thetvdb")), QStringLiteral("https://thetvdb.com/series/firefly"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Science Fiction; Drama; Adventure"));
  QVERIFY(entry->field(QStringLiteral("director")).startsWith(QStringLiteral("Joss Whedon; ")));
  QVERIFY(entry->field(QStringLiteral("writer")).contains(QStringLiteral("Joss Whedon")));
  QVERIFY(entry->field(QStringLiteral("writer")).contains(QStringLiteral("Tim Minear")));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(castList.size() > 2);
  QCOMPARE(castList.at(0), QStringLiteral("Adam Baldwin::Jayne Cobb"));
  QCOMPARE(castList.at(2), QStringLiteral("Gina Torres::Zoë Washburne"));
  QStringList episodeList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("episode")));
  QVERIFY(!episodeList.isEmpty());
  QCOMPARE(episodeList.at(0), QStringLiteral("The Train Job::1::1"));
  QCOMPARE(entry->field(QStringLiteral("imdb")), QStringLiteral("https://www.imdb.com/title/tt0303461"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void TheTVDBFetcherTest::testUpdate() {
  QSKIP("This test is completely broken right now since API v4 doesn't have any of this", SkipAll);
  const QString groupName = QStringLiteral("TheTVDB");
  if(!m_hasConfigFile || !m_config->hasGroup(groupName)) {
    QSKIP("This test requires a config file with TheTVDB settings.", SkipAll);
  }
  KConfigGroup cg(m_config, groupName);

  Tellico::Data::CollPtr coll(new Tellico::Data::VideoCollection(true));
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("thetvdb"),
                                                         QStringLiteral("TheTVDB"),
                                                         Tellico::Data::Field::URL));
  QCOMPARE(coll->addField(field), true);
  field = new Tellico::Data::Field(QStringLiteral("imdb"),
                                   QStringLiteral("IMDB"),
                                   Tellico::Data::Field::URL);
  QCOMPARE(coll->addField(field), true);
  Tellico::Data::EntryPtr oldEntry(new Tellico::Data::Entry(coll));
  coll->addEntries(oldEntry);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheTVDBFetcher(this));
  auto f = static_cast<Tellico::Fetch::TheTVDBFetcher*>(fetcher.data());

  oldEntry->setField(QStringLiteral("thetvdb"), QStringLiteral("https://thetvdb.com/series/firefly"));
  auto request = f->updateRequest(oldEntry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QCOMPARE(request.value(), QStringLiteral("firefly"));
  QCOMPARE(request.data(), QStringLiteral("slug"));

  oldEntry->setField(QStringLiteral("imdb"), QStringLiteral("https://www.imdb.com/title/tt0303461/?ref_=nv_sr_srsg_0"));
  request = f->updateRequest(oldEntry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QCOMPARE(request.value(), QStringLiteral("tt0303461"));
  QCOMPARE(request.data(), QStringLiteral("imdb"));

  fetcher->readConfig(cg);

  request.setCollectionType(coll->type());
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Firefly"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2002"));
  QCOMPARE(entry->field(QStringLiteral("network")), QStringLiteral("FOX"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("English"));
  QCOMPARE(entry->field(QStringLiteral("certification")), QStringLiteral("TV-14"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Drama; Science Fiction"));
  QCOMPARE(entry->field(QStringLiteral("imdb")), QStringLiteral("https://www.imdb.com/title/tt0303461"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}
