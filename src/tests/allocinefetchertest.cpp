/***************************************************************************
    Copyright (C) 2010-2011 Robby Stephenson <robby@periapsis.org>
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

#include "allocinefetchertest.h"
#include "allocinefetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/execexternalfetcher.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KStandardDirs>
#include <KConfigGroup>

QTEST_KDEMAIN( AllocineFetcherTest, GUI )

AllocineFetcherTest::AllocineFetcherTest() : AbstractFetcherTest() {
}

void AllocineFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerGame(Tellico::Data::Collection::Video, "video");
  // since we use the importer
//  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();
}

void AllocineFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QLatin1String("Superman Returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/fr.allocine.py.spec", KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QLatin1String("<default>"));
  cg.writeEntry("ExecPath", QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/fr.allocine.py");
  // don't sync() and save the new path
  cg.markAsClean();
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Superman Returns"));
  QCOMPARE(entry->field(QLatin1String("director")), QLatin1String("Bryan Singer"));
  QCOMPARE(entry->field(QLatin1String("producer")), QLatin1String("Jon Peters; Gilbert Adler; Bryan Singer; Lorne Orleans"));
  QCOMPARE(entry->field(QLatin1String("studio")), QLatin1String("Warner Bros. France"));
  QCOMPARE(entry->field(QLatin1String("year")), QLatin1String("2006"));
  QCOMPARE(entry->field(QLatin1String("genre")), QLatin1String("Fantastique; Action"));
  QCOMPARE(entry->field(QLatin1String("nationality")), QLatin1String("Américain; Australien"));
  QCOMPARE(entry->field(QLatin1String("running-time")), QLatin1String("154"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QLatin1String("Clark Kent / Superman::Brandon Routh"));
  QCOMPARE(castList.size(), 8);
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}

void AllocineFetcherTest::testTitleAccented() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QLatin1String("Opération Tonnerre"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/fr.allocine.py.spec", KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QLatin1String("<default>"));
  cg.writeEntry("ExecPath", QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/fr.allocine.py");
  // don't sync() and save the new path
  cg.markAsClean();
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Opération Tonnerre"));
  QCOMPARE(entry->field(QLatin1String("titre-original")), QLatin1String("Thunderball"));
  QCOMPARE(entry->field(QLatin1String("studio")), QLatin1String(""));
}

void AllocineFetcherTest::testTitleAccentRemoved() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QLatin1String("Operation Tonnerre"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/fr.allocine.py.spec", KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QLatin1String("<default>"));
  cg.writeEntry("ExecPath", QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/fr.allocine.py");
  // don't sync() and save the new path
  cg.markAsClean();
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Opération Tonnerre"));
}

void AllocineFetcherTest::testPlotQuote() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QLatin1String("Goldfinger"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/fr.allocine.py.spec", KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QLatin1String("<default>"));
  cg.writeEntry("ExecPath", QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/fr.allocine.py");
  // don't sync() and save the new path
  cg.markAsClean();
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Goldfinger"));
  QVERIFY(!entry->field(QLatin1String("plot")).contains(QLatin1String("&quot;")));
}
