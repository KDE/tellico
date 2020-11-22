/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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

#include "tvmazefetchertest.h"

#include "../fetch/tvmazefetcher.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( TVmazeFetcherTest )

TVmazeFetcherTest::TVmazeFetcherTest() : AbstractFetcherTest() {
}

void TVmazeFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void TVmazeFetcherTest::testTitle() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("tvmaze"));
  cg.writeEntry("Custom Fields", QStringLiteral("imdb,episode,alttitle,network"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("Firefly"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TVmazeFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Firefly"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2002"));
  QCOMPARE(entry->field(QStringLiteral("network")), QStringLiteral("FOX"));
  QCOMPARE(entry->field(QStringLiteral("language")), QStringLiteral("English"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Drama; Adventure; Science-Fiction"));
  QCOMPARE(entry->field(QStringLiteral("writer")), QStringLiteral("Joss Whedon"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QStringLiteral("Nathan Fillion::Captain Malcolm \"Mal\" Reynolds"));
  QStringList episodeList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("episode")));
  QVERIFY(!episodeList.isEmpty());
  QCOMPARE(episodeList.at(0), QStringLiteral("The Train Job::1::1"));
  QCOMPARE(entry->field(QStringLiteral("imdb")), QStringLiteral("https://www.imdb.com/title/tt0303461"));
  QVERIFY(!entry->field(QStringLiteral("alttitle")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}
