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

#include "masfetchertest.h"
#include "masfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/masfetcher.h"
#include "../entry.h"

QTEST_KDEMAIN( MASFetcherTest, GUI )

MASFetcherTest::MASFetcherTest() : AbstractFetcherTest() {
}

void MASFetcherTest::initTestCase() {
  m_fieldValues.insert(QLatin1String("title"),
                       QLatin1String("Tropospheric emission spectrometer: "
                                     "retrieval method and error analysis"));
  m_fieldValues.insert(QLatin1String("journal"),
                       QLatin1String("IEEE Transactions on Geoscience and Remote Sensing"));
  m_fieldValues.insert(QLatin1String("doi"), QLatin1String("10.1109/TGRS.2006.871234"));
  m_fieldValues.insert(QLatin1String("url"),
                       QLatin1String("http://aura.gsfc.nasa.gov/science/publications/"
                                     "Bowman_Rogers_Kulawik_IEEE2006.pdf"));
  m_fieldValues.insert(QLatin1String("year"), QLatin1String("2006"));
}

void MASFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Title,
                                       m_fieldValues.value(QLatin1String("title")));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MASFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(entry->field(QLatin1String("author")).startsWith(QLatin1String("Kevin W. Bowman")));
  QVERIFY(entry->field(QLatin1String("keyword")).contains(QLatin1String("Surface Temperature")));
}

void MASFetcherTest::testAuthor() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Person,
                                       QLatin1String("Kevin W. Bowman"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MASFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);
  // find this specific paper, hoping it's in the first 20
  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr tmpEntry, results) {
    if(tmpEntry->field(QLatin1String("doi")) == m_fieldValues.value(QLatin1String("doi"))) {
      entry = tmpEntry;
      break;
    } else {
//      qDebug() << "Skipping" << tmpEntry->title();
    }
  }

  QVERIFY(entry);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(entry->field(QLatin1String("author")).startsWith(QLatin1String("Kevin W. Bowman")));
  QVERIFY(entry->field(QLatin1String("keyword")).contains(QLatin1String("Surface Temperature")));
}

void MASFetcherTest::testKeyword() {
  // for keyword search, just do title again
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::Keyword,
                                       m_fieldValues.value(QLatin1String("title")));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::MASFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(entry->field(QLatin1String("author")).startsWith(QLatin1String("Kevin W. Bowman")));
  QVERIFY(entry->field(QLatin1String("keyword")).contains(QLatin1String("Surface Temperature")));
}

