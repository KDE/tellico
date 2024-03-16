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

#include "filmasterfetchertest.h"

#include "../fetch/fetcherjob.h"
#include "../fetch/filmasterfetcher.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <QTest>

QTEST_MAIN( FilmasterFetcherTest )

FilmasterFetcherTest::FilmasterFetcherTest() : AbstractFetcherTest() {
}

void FilmasterFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("The Man from Snowy River"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("1982"));
  m_fieldValues.insert(QStringLiteral("genre"), QStringLiteral("western; romans; familijny; dramat"));
  m_fieldValues.insert(QStringLiteral("director"), QStringLiteral("George Miller"));
//  m_fieldValues.insert(QStringLiteral("nationality"), QStringLiteral("Australia"));
}

void FilmasterFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("Man From Snowy River"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FilmasterFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 2);

  QCOMPARE(results.size(), 2);

  Tellico::Data::EntryPtr entry = results.at(1);
  QVERIFY(entry);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QCOMPARE(castList.at(0), QStringLiteral("Tom Burlinson::Jim Craig"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
//   2017-05-01: plot is no longer being returned
//  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void FilmasterFetcherTest::testPerson() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Person,
                                       QStringLiteral("Tom Burlinson"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FilmasterFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(results.size() > 0);
  Tellico::Data::EntryPtr entry;  //  results can be randomly ordered, loop until wee find the one we want
  for(int i = 0; i < results.size(); ++i) {
    Tellico::Data::EntryPtr test = results.at(i);
    if(test->field(QStringLiteral("title")).toLower() == QStringLiteral("the man from snowy river")) {
      entry = test;
      break;
    } else {
      qDebug() << "skipping" << test->title();
    }
  }
  QVERIFY(entry);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QCOMPARE(castList.at(0), QStringLiteral("Tom Burlinson::Jim Craig"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
//   2017-05-01: plot is no longer being returned
//  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void FilmasterFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword,
                                       QStringLiteral("Man From Snowy River"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::FilmasterFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 2);

  QCOMPARE(results.size(), 2);

  Tellico::Data::EntryPtr entry = results.at(1);
  QVERIFY(entry);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QCOMPARE(castList.at(0), QStringLiteral("Tom Burlinson::Jim Craig"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
//   2017-05-01: plot is no longer being returned
//  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}
