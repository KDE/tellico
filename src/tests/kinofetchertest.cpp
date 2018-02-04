/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include "kinofetchertest.h"

#include "../fetch/kinofetcher.h"
#include "../entry.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"
#include "../fetch/fetcherjob.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( KinoFetcherTest )

KinoFetcherTest::KinoFetcherTest() : AbstractFetcherTest() {
}

void KinoFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
}

void KinoFetcherTest::testTitle() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("IMDB");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Superman Returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::KinoFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("Superman Returns"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(set(entry, "genre"), set("Superhelden-Film; Comicverfilmung; Fantasyfilm; Actionfilm"));
  QCOMPARE(entry->field("director"), QStringLiteral("Bryan Singer"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QCOMPARE(castList.size(), 9);
  QCOMPARE(castList.at(0), QStringLiteral("Brandon Routh"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("USA"));
  QCOMPARE(entry->field("studio"), QStringLiteral("Warner"));
  QCOMPARE(entry->field("running-time"), QStringLiteral("154"));
  QCOMPARE(entry->field("certification"), QStringLiteral("FSK 12 (DE)"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
