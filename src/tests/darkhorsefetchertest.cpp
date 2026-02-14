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

#include "darkhorsefetchertest.h"

#include "../fetch/execexternalfetcher.h"
#include "../entry.h"
#include "../collections/comicbookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../fieldformat.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( DarkHorseFetcherTest )

#define QSL(x) QStringLiteral(x)

DarkHorseFetcherTest::DarkHorseFetcherTest() : AbstractFetcherTest() {
}

void DarkHorseFetcherTest::initTestCase() {
  const QString python = QStandardPaths::findExecutable(QSL("python"));
  if(python.isEmpty()) {
    QSKIP("This test requires python", SkipAll);
  }

  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::ComicBookCollection> registerComic(Tellico::Data::Collection::ComicBook, "comicbook");
}

void DarkHorseFetcherTest::testComic() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::Title,
                                       QSL("axe cop: bad guy earth #1"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KSharedConfig::Ptr config = KSharedConfig::openConfig(QFINDTESTDATA("../fetch/scripts/dark_horse_comics.py.spec"),
                                                        KConfig::SimpleConfig);
  KConfigGroup cg = config->group(QSL("<default>"));
  cg.writeEntry("ExecPath", QFINDTESTDATA("../fetch/scripts/dark_horse_comics.py"));
  fetcher->readConfig(cg);
  // don't sync() and save the new path
  cg.deleteEntry("ExecPath");

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);
  QVERIFY(results.size() > 0);

  int n = 0;
  Tellico::Data::EntryPtr entry;
  do {
    entry = results.at(n++);
  } while(n < results.size() &&
          entry->title() != QLatin1String("Axe Cop: Bad Guy Earth #1"));

  QCOMPARE(entry->field(QSL("title")), QSL("Axe Cop: Bad Guy Earth #1"));
  QCOMPARE(entry->field(QSL("pub_year")), QSL("2011"));
  QCOMPARE(set(entry->field(QSL("genre"))), set(QSL("Kids; Humor")));
  QCOMPARE(entry->field(QSL("pages")), QSL("32"));
  QCOMPARE(entry->field(QSL("issue")), QSL("1"));
  QCOMPARE(entry->field(QSL("publisher")), QSL("Dark Horse Comics"));
  QCOMPARE(entry->field(QSL("writer")), QSL("Malachai Nicolle"));
  QCOMPARE(entry->field(QSL("artist")), QSL("Ethan Nicolle"));
  QVERIFY(!entry->field(QSL("comments")).isEmpty());
  QVERIFY(!entry->field(QSL("cover")).isEmpty());
  QVERIFY(!entry->field(QSL("cover")).contains(QLatin1Char('/')));
  QVERIFY(!Tellico::ImageFactory::imageById(entry->field(QSL("cover"))).isNull());
}

void DarkHorseFetcherTest::testMasterverse() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::Title,
                                       QSL("Masters of the Universe: Masterverse #1"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KSharedConfig::Ptr config = KSharedConfig::openConfig(QFINDTESTDATA("../fetch/scripts/dark_horse_comics.py.spec"),
                                                        KConfig::SimpleConfig);
  KConfigGroup cg = config->group(QSL("<default>"));
  cg.writeEntry("ExecPath", QFINDTESTDATA("../fetch/scripts/dark_horse_comics.py"));
  fetcher->readConfig(cg);
  // don't sync() and save the new path
  cg.deleteEntry("ExecPath");

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);
  QVERIFY(results.size() > 0);

  int n = 0;
  Tellico::Data::EntryPtr entry;
  do {
    entry = results.at(n++);
  } while(n < results.size() &&
          entry->title() != QLatin1String("Masters of the Universe: Masterverse #1"));

  QCOMPARE(entry->field(QSL("title")), QSL("Masters of the Universe: Masterverse #1"));
  QCOMPARE(entry->field(QSL("pub_year")), QSL("2023"));
  QCOMPARE(set(entry->field(QSL("genre"))), set(QSL("Superhero; Action/Adventure")));
  QCOMPARE(entry->field(QSL("pages")), QSL("32"));
  QCOMPARE(entry->field(QSL("publisher")), QSL("Dark Horse Comics"));
  QCOMPARE(entry->field(QSL("writer")), QSL("Tim Seeley"));
  QCOMPARE(set(entry->field(QSL("artist"))), set(QSL("Eddie Nunez; Sergio Aragon\u00C3\u00A9s; Kelley Jones; Rico Renzi; Brennan Wagner")));
  QCOMPARE(entry->field(QSL("darkhorse")), QSL("https://www.darkhorse.com/comics/3010-272/masters-of-the-universe-masterverse-1/"));
}
