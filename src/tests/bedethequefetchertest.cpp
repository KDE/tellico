/***************************************************************************
    Copyright (C) 2016-2020 Robby Stephenson <robby@periapsis.org>
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

#include "../fetch/bedethequefetcher.h"
#include "../entry.h"
#include "../collections/comicbookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( BedethequeFetcherTest )

BedethequeFetcherTest::BedethequeFetcherTest() : AbstractFetcherTest() {
}

void BedethequeFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::ComicBookCollection> registerCB(Tellico::Data::Collection::ComicBook, "comic");

  m_config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("bedetheque"));
  m_config.writeEntry("Custom Fields", QStringLiteral("isbn,lien-bel,colorist,comments"));
}

void BedethequeFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::Title,
                                       QStringLiteral("Le Combat d'Odiri"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BedethequeFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("Le Combat d'Odiri"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("1991"));
  QCOMPARE(entry->field("series"), QStringLiteral("(AUT) Arno"));
  QCOMPARE(entry->field("writer"), QString::fromUtf8("Châteaureynaud, Georges-Olivier"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Bayard"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Arno (1)"));
  QCOMPARE(entry->field("colorist"), QStringLiteral("Arno (1)"));
  QCOMPARE(entry->field("pages"), QStringLiteral("88"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Tout sur un auteur (hors BD)"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("2-227-72311-4"));
  QCOMPARE(entry->field("edition"), QString::fromUtf8("Je bouquine : à partir de 10 ans"));
  QCOMPARE(entry->field("lien-bel"), QStringLiteral("https://www.bedetheque.com/BD-AUT-Arno-Le-Combat-d-Odiri-46179.html"));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void BedethequeFetcherTest::testSeries() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::Keyword,
                                       QStringLiteral("Arno"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BedethequeFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("Le Combat d'Odiri"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("1991"));
  QCOMPARE(entry->field("series"), QStringLiteral("(AUT) Arno"));
  QCOMPARE(entry->field("writer"), QString::fromUtf8("Châteaureynaud, Georges-Olivier"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Bayard"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Arno (1)"));
  QCOMPARE(entry->field("colorist"), QStringLiteral("Arno (1)"));
  QCOMPARE(entry->field("pages"), QStringLiteral("88"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Tout sur un auteur (hors BD)"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("2-227-72311-4"));
  QCOMPARE(entry->field("edition"), QString::fromUtf8("Je bouquine : à partir de 10 ans"));
  QCOMPARE(entry->field("lien-bel"), QStringLiteral("https://www.bedetheque.com/BD-AUT-Arno-Le-Combat-d-Odiri-46179.html"));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void BedethequeFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::ISBN,
                                       QStringLiteral("2-205-05868-1"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BedethequeFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QString::fromUtf8("Jérusalem d'Afrique"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("series"), QStringLiteral("Chat du Rabbin (Le)"));
  QCOMPARE(entry->field("writer"), QStringLiteral("Sfar, Joann"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Sfar, Joann"));
  QCOMPARE(entry->field("colorist"), QStringLiteral("Findakly, Brigitte"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Dargaud"));
  QCOMPARE(entry->field("pages"), QStringLiteral("80"));
  QCOMPARE(entry->field("issue"), QStringLiteral("5"));
  QCOMPARE(entry->field("edition"), QStringLiteral("Poisson Pilote"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Aventure"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("2-205-05868-1"));
  QCOMPARE(entry->field("lien-bel"), QStringLiteral("https://www.bedetheque.com/BD-Chat-du-Rabbin-Tome-5-Jerusalem-d-Afrique-59668.html"));
  QVERIFY(!entry->field("comments").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void BedethequeFetcherTest::testDonjon() {
  // this one has multiple writers
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::ComicBook, Tellico::Fetch::Raw,
                                       QStringLiteral("http://m.bedetheque.com/BD-Donjon-Zenith-Tome-5-Un-mariage-a-part-56495.html"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BedethequeFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QString::fromUtf8("Un mariage à part"));
  QCOMPARE(set(entry, "writer"), set("Sfar, Joann; Boulet; Trondheim, Lewis"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Boulet"));
  QCOMPARE(entry->field("colorist"), QStringLiteral("Albon, Lucie"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("2-84055-734-7"));
  QCOMPARE(entry->field("lien-bel"), QStringLiteral("https://www.bedetheque.com/BD-Donjon-Zenith-Tome-5-Un-mariage-a-part-56495.html"));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void BedethequeFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::ComicBookCollection(true));
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("lien-bel"),
                                                         QStringLiteral("Link"),
                                                         Tellico::Data::Field::URL));
  coll->addField(field);
  Tellico::Data::EntryPtr oldEntry(new Tellico::Data::Entry(coll));
  coll->addEntries(oldEntry);

  oldEntry->setField(QStringLiteral("lien-bel"), QStringLiteral("https://www.bedetheque.com/BD-Donjon-Zenith-Tome-5-Un-mariage-a-part-56495.html"));

  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::BedethequeFetcher(this));
  auto f = static_cast<Tellico::Fetch::BedethequeFetcher*>(fetcher.data());

  auto request = f->updateRequest(oldEntry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QCOMPARE(request.value(), oldEntry->field(QStringLiteral("lien-bel")));
}
