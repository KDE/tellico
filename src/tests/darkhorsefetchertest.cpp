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

#undef QT_NO_CAST_FROM_ASCII

#include "darkhorsefetchertest.h"
#include "darkhorsefetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/execexternalfetcher.h"
#include "../entry.h"
#include "../collections/comicbookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../fieldformat.h"

#include <KConfigGroup>
#include <KStandardDirs>

QTEST_KDEMAIN( DarkHorseFetcherTest, GUI )

DarkHorseFetcherTest::DarkHorseFetcherTest() : AbstractFetcherTest() {
}

void DarkHorseFetcherTest::initTestCase() {
  const QString python = KStandardDirs::findExe(QLatin1String("python"));
  if(python.isEmpty()) {
    QSKIP("This test requires python", SkipAll);
  }

  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::ComicBookCollection> registerComic(Tellico::Data::Collection::ComicBook, "comicbook");
}

void DarkHorseFetcherTest::testComic() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::Title,
                                       QLatin1String("axe cop: bad guy earth"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::ExecExternalFetcher(this));

  KConfig config(QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/dark_horse_comics.py.spec", KConfig::SimpleConfig);
  KConfigGroup cg = config.group(QLatin1String("<default>"));
  cg.writeEntry("ExecPath", QString::fromLatin1(KDESRCDIR) + "/../fetch/scripts/dark_horse_comics.py");
  // don't sync() and save the new path
  cg.markAsClean();
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QLatin1String("Axe Cop: Bad Guy Earth #1"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("2011"));
  QCOMPARE(entry->field("genre"), QLatin1String("Humor"));
  QCOMPARE(entry->field("pages"), QLatin1String("32"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Dark Horse Comics"));
  QCOMPARE(entry->field("writer"), QLatin1String("Malachai Nicolle"));
  QCOMPARE(entry->field("artist"), QLatin1String("Ethan Nicolle"));
  QVERIFY(!entry->field("comments").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!Tellico::ImageFactory::imageById(entry->field("cover")).isNull());
}
