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

#include "kinopoiskfetchertest.h"

#include "../fetch/kinopoiskfetcher.h"
#include "../entry.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"
#include "../fetch/fetcherjob.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( KinoPoiskFetcherTest )

KinoPoiskFetcherTest::KinoPoiskFetcherTest() : AbstractFetcherTest() {
}

void KinoPoiskFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
}

void KinoPoiskFetcherTest::testSuperman() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("kinopoisk");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Superman Returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::KinoPoiskFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QString::fromUtf8("Возвращение Супермена"));
  QCOMPARE(entry->field("origtitle"), QStringLiteral("Superman Returns"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("nationality"), QString::fromUtf8("США"));
  QCOMPARE(entry->field("director"), QString::fromUtf8("Брайан Сингер"));
  QCOMPARE(entry->field("writer"), QString::fromUtf8("Майкл Догерти; Дэн Харрис; Брайан Сингер"));
  QCOMPARE(entry->field("producer"), QString::fromUtf8("Гилберт Адлер; Джон Питерс; Брайан Сингер"));
  QCOMPARE(entry->field("composer"), QString::fromUtf8("Джон Оттмен"));
  QCOMPARE(set(entry, "genre"), set(QString::fromUtf8("фантастика; боевик; приключения")));
  QCOMPARE(entry->field("certification"), QStringLiteral("PG-13 (USA)"));
  QCOMPARE(entry->field("running-time"), QStringLiteral("154"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QString::fromUtf8("Брэндон Рут"));
//  QVERIFY(entry->field("plot").startsWith(QString::fromUtf8("Возвратившись на Землю")));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void KinoPoiskFetcherTest::testTop() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Top"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::KinoPoiskFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QString::fromUtf8("Top"));
  QCOMPARE(entry->field("year"), QStringLiteral("1999"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
