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

#include "moviemeterfetchertest.h"

#include "../fetch/moviemeterfetcher.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <QTest>

QTEST_GUILESS_MAIN( MovieMeterFetcherTest )

MovieMeterFetcherTest::MovieMeterFetcherTest() : AbstractFetcherTest() {
}

void MovieMeterFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QLatin1String("title"), QLatin1String("Man from Snowy River, The"));
  m_fieldValues.insert(QLatin1String("year"), QLatin1String("1982"));
  m_fieldValues.insert(QLatin1String("director"), QLatin1String("George Miller"));
  m_fieldValues.insert(QLatin1String("running-time"), QLatin1String("102"));
  m_fieldValues.insert(QLatin1String("genre"), QLatin1String("Western"));
  m_fieldValues.insert(QLatin1String("nationality"), QString::fromUtf8("Australië"));
}

void MovieMeterFetcherTest::testKeyword() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword,
                                       QLatin1String("Man From Snowy River"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MovieMeterFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 2);

  QCOMPARE(results.size(), 2);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QLatin1String("Tom Burlinson"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
}

void MovieMeterFetcherTest::testKeywordCzech() {
//  QString tmav = QString::fromUtf8("Tmavomodrý Svět");
  QString tmav = QString::fromUtf8("Tmavomodrý Svet");
//  QString tmav = QString::fromUtf8("Tmavomodrý svet");
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Keyword,
                                       tmav);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MovieMeterFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 2);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);

  QCOMPARE(entry->title(), tmav);
}
