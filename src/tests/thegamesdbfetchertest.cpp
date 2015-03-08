/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "thegamesdbfetchertest.h"
#include "qtest_kde.h"

#include "../fetch/thegamesdbfetcher.h"
#include "../collections/gamecollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KStandardDirs>

QTEST_KDEMAIN( TheGamesDBFetcherTest, GUI )

TheGamesDBFetcherTest::TheGamesDBFetcherTest() : AbstractFetcherTest() {
}

void TheGamesDBFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::GameCollection> registerGame(Tellico::Data::Collection::Game, "game");
  // since we use the importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QLatin1String("title"), QLatin1String("GoldenEye 007"));
  m_fieldValues.insert(QLatin1String("platform"), QLatin1String("Nintendo 64"));
  m_fieldValues.insert(QLatin1String("year"), QLatin1String("1997"));
  m_fieldValues.insert(QLatin1String("certification"), QLatin1String("Teen"));
  m_fieldValues.insert(QLatin1String("genre"), QLatin1String("Shooter"));
  m_fieldValues.insert(QLatin1String("publisher"), QLatin1String("Nintendo"));
  m_fieldValues.insert(QLatin1String("developer"), QLatin1String("Rareware"));
}

void TheGamesDBFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Title,
                                       QLatin1String("Goldeneye"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheGamesDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QLatin1String("description")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}

void TheGamesDBFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Keyword,
                                       QLatin1String("Goldeneye"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheGamesDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QLatin1String("description")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}
