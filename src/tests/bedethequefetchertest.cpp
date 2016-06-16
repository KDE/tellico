/***************************************************************************
    Copyright (C) 2016 Robby Stephenson <robby@periapsis.org>
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

#include "bedethequefetchertest.h"
#include "bedethequefetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/bedethequefetcher.h"
#include "../entry.h"
#include "../collections/comicbookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"
#include "../fetch/fetcherjob.h"

#include <KConfigGroup>

QTEST_KDEMAIN( BedethequeFetcherTest, GUI )

BedethequeFetcherTest::BedethequeFetcherTest() : AbstractFetcherTest() {
}

void BedethequeFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::ComicBookCollection> registerCB(Tellico::Data::Collection::ComicBook, "comic");
}

void BedethequeFetcherTest::testSeriesArno() {
  KConfig config(QString::fromLatin1(KDESRCDIR)  + "/tellicotest.config", KConfig::SimpleConfig);
  QString groupName = QLatin1String("bedetheque");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::Keyword, "Arno");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BedethequeFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QLatin1String("Le Combat d'Odiri"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("1991"));
  QCOMPARE(entry->field("series"), QLatin1String("(AUT) Arno"));
  QCOMPARE(entry->field("writer"), QString::fromLatin1("Châteaureynaud, Georges-Olivier"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Bayard"));
  QCOMPARE(entry->field("artist"), QLatin1String("Arno"));
  QCOMPARE(entry->field("colorist"), QLatin1String("Arno"));
  QCOMPARE(entry->field("pages"), QLatin1String("88"));
  QCOMPARE(entry->field("genre"), QLatin1String("Tout sur un auteur (hors BD)"));
  QCOMPARE(entry->field("isbn"), QLatin1String("2-227-72311-4"));
  QCOMPARE(entry->field("edition"), QString::fromLatin1("Je bouquine : à partir de 10 ans"));
  QCOMPARE(entry->field("lien-bel"), QLatin1String("http://www.bedetheque.com/BD-AUT-Arno-Le-Combat-d-Odiri-46179.html"));
  QVERIFY(!entry->field("comments").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
}

void BedethequeFetcherTest::testIsbn() {
  KConfig config(QString::fromLatin1(KDESRCDIR)  + "/tellicotest.config", KConfig::SimpleConfig);
  QString groupName = QLatin1String("bedetheque");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::ISBN, "2-205-05868-1");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BedethequeFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QString::fromLatin1("Jérusalem d'Afrique"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("2006"));
  QCOMPARE(entry->field("series"), QLatin1String("Chat du Rabbin (Le)"));
  QCOMPARE(entry->field("writer"), QLatin1String("Sfar, Joann"));
  QCOMPARE(entry->field("artist"), QLatin1String("Sfar, Joann"));
  QCOMPARE(entry->field("colorist"), QLatin1String("Findakly, Brigitte"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Dargaud"));
  QCOMPARE(entry->field("pages"), QLatin1String("80"));
  QCOMPARE(entry->field("issue"), QLatin1String("5"));
  QCOMPARE(entry->field("edition"), QLatin1String("Poisson Pilote"));
  QCOMPARE(entry->field("genre"), QLatin1String("Aventure"));
  QCOMPARE(entry->field("isbn"), QLatin1String("2-205-05868-1"));
  QCOMPARE(entry->field("lien-bel"), QLatin1String("http://www.bedetheque.com/BD-Chat-du-Rabbin-Tome-5-Jerusalem-d-Afrique-59668.html"));
  QVERIFY(!entry->field("cover").isEmpty());
}

void BedethequeFetcherTest::testDonjon() {
  // this one has multiple writers
  KConfig config(QString::fromLatin1(KDESRCDIR)  + "/tellicotest.config", KConfig::SimpleConfig);
  QString groupName = QLatin1String("bedetheque");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::Raw, "http://m.bedetheque.com/BD-Donjon-Zenith-Tome-5-Un-mariage-a-part-56495.html");
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BedethequeFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QString::fromLatin1("Un mariage à part"));
  QCOMPARE(entry->field("writer"), QLatin1String("Sfar, Joann; Boulet; Trondheim, Lewis"));
  QCOMPARE(entry->field("artist"), QLatin1String("Boulet"));
  QCOMPARE(entry->field("colorist"), QLatin1String("Albon, Lucie"));
  QCOMPARE(entry->field("isbn"), QLatin1String("2-84055-734-7"));
  QCOMPARE(entry->field("lien-bel"), QLatin1String("http://www.bedetheque.com/BD-Donjon-Zenith-Tome-5-Un-mariage-a-part-56495.html"));
  QVERIFY(!entry->field("cover").isEmpty());
}
