/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#include "filmaffinityfetchertest.h"

#include "../fetch/filmaffinityfetcher.h"
#include "../entry.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( FilmAffinityFetcherTest )

FilmAffinityFetcherTest::FilmAffinityFetcherTest() : AbstractFetcherTest() {
}

void FilmAffinityFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");

  m_config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("filmaffinity"));
  m_config.writeEntry("Custom Fields", QStringLiteral("origtitle,filmaffinity"));
}

void FilmAffinityFetcherTest::testSuperman() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Superman Returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FilmAffinityFetcher(this));
  m_config.writeEntry("Locale", int(Tellico::Fetch::FilmAffinityFetcher::US));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // verify the search results contain no html
  QVERIFY(!m_resultTitles.isEmpty());
  QCOMPARE(m_resultTitles.at(0), QStringLiteral("Superman Returns"));

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("Superman Returns"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("United States"));
  QCOMPARE(entry->field("director"), QStringLiteral("Bryan Singer"));
  QCOMPARE(set(entry->field("writer")), set(QStringLiteral("Bryan Singer; Michael Dougherty; Dan Harris")));
  QCOMPARE(entry->field("composer"), QStringLiteral("John Ottman"));
  QCOMPARE(entry->field("studio"), QStringLiteral("Warner Bros."));
  QCOMPARE(set(entry, "genre"), set(QStringLiteral("Sci-Fi; Fantasy; Action; Romance")));
  QCOMPARE(entry->field("running-time"), QStringLiteral("153"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(castList.count() > 2);
  QCOMPARE(castList.at(0), QStringLiteral("Brandon Routh"));
  QCOMPARE(castList.at(1), QStringLiteral("Kevin Spacey"));
  QVERIFY(entry->field("plot").startsWith(QStringLiteral("Following a mysterious absence")));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QCOMPARE(entry->field("filmaffinity"), QStringLiteral("https://www.filmaffinity.com/us/film300630.html"));
}

void FilmAffinityFetcherTest::testSupermanES() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Superman Returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FilmAffinityFetcher(this));
  m_config.writeEntry("Locale", int(Tellico::Fetch::FilmAffinityFetcher::ES));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("Superman Returns: El regreso"));
  QCOMPARE(entry->field("origtitle"), QStringLiteral("Superman Returns"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("Estados Unidos"));
  QCOMPARE(entry->field("director"), QStringLiteral("Bryan Singer"));
  QCOMPARE(set(entry->field("writer")), set(QStringLiteral("Bryan Singer; Michael Dougherty; Dan Harris")));
  QCOMPARE(entry->field("composer"), QStringLiteral("John Ottman"));
  QCOMPARE(entry->field("studio"), QStringLiteral("Warner Bros."));
  QCOMPARE(set(entry, "genre"), set(QString::fromUtf8("Ciencia ficción; Fantástico; Acción; Romance")));
  QCOMPARE(entry->field("running-time"), QStringLiteral("153"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(castList.count() > 2);
  QCOMPARE(castList.at(0), QStringLiteral("Brandon Routh"));
  QCOMPARE(castList.at(1), QStringLiteral("Kevin Spacey"));
  QVERIFY(entry->field("plot").startsWith(QString::fromUtf8("Tras varios años")));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QCOMPARE(entry->field("filmaffinity"), QStringLiteral("https://www.filmaffinity.com/es/film300630.html"));
}

void FilmAffinityFetcherTest::testFirefly() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Firefly 2002"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FilmAffinityFetcher(this));
  m_config.writeEntry("Locale", int(Tellico::Fetch::FilmAffinityFetcher::US));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("Firefly"));
  QCOMPARE(entry->field("year"), QStringLiteral("2002"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("United States"));
  QCOMPARE(entry->field("studio"), QStringLiteral("FOX"));
  QVERIFY(!entry->field("plot").isEmpty());
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QCOMPARE(entry->field("filmaffinity"), QStringLiteral("https://www.filmaffinity.com/us/film929343.html"));
}

void FilmAffinityFetcherTest::testAlcarras() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Alcarràs"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FilmAffinityFetcher(this));
  m_config.writeEntry("Locale", int(Tellico::Fetch::FilmAffinityFetcher::ES));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QString::fromUtf8("Alcarràs"));
  QCOMPARE(entry->field("year"), QStringLiteral("2022"));
  QCOMPARE(entry->field("nationality"), QString::fromUtf8("España"));
  QCOMPARE(entry->field("director"), QString::fromUtf8("Carla Simón"));
  QCOMPARE(entry->field("writer"), QString::fromUtf8("Carla Simón; Arnau Vilaró"));
  QCOMPARE(entry->field("composer"), QStringLiteral("Andrea Koch"));
  QCOMPARE(entry->field("studio"), QString::fromUtf8("Avalon; Elastica Films; Vilaüt Films; Kino Produzioni; Movistar Plus+; RTVE; TV3"));
  QCOMPARE(set(entry, "genre"), set(QStringLiteral("Drama")));
  QCOMPARE(entry->field("running-time"), QStringLiteral("120"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(castList.count() > 2);
  QCOMPARE(castList.at(0), QStringLiteral("Jordi Pujol Dolcet"));
  QCOMPARE(castList.at(1), QStringLiteral("Anna Otín"));
  QVERIFY(entry->field("plot").startsWith(QStringLiteral("La familia Solé")));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QCOMPARE(entry->field("filmaffinity"), QStringLiteral("https://www.filmaffinity.com/es/film457848.html"));
}
