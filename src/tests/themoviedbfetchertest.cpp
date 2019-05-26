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

#include "themoviedbfetchertest.h"

#include "../fetch/themoviedbfetcher.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( TheMovieDBFetcherTest )

TheMovieDBFetcherTest::TheMovieDBFetcherTest() : AbstractFetcherTest() {
}

void TheMovieDBFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("Superman Returns"));
  m_fieldValues.insert(QStringLiteral("studio"), QStringLiteral("Warner Bros. Pictures; Red Sun Productions Pty. Ltd.; "
                                                                "Peters Entertainment; DC Comics; Legendary Entertainment; "
                                                                "Bad Hat Harry Productions"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("2006"));
  m_fieldValues.insert(QStringLiteral("genre"), QStringLiteral("action; adventure; fantasy; science fiction"));
  m_fieldValues.insert(QStringLiteral("director"), QStringLiteral("Bryan Singer"));
  m_fieldValues.insert(QStringLiteral("producer"), QStringLiteral("Bryan Singer; Jon Peters; Gilbert Adler"));
  m_fieldValues.insert(QStringLiteral("running-time"), QStringLiteral("154"));
  m_fieldValues.insert(QStringLiteral("nationality"), QStringLiteral("USA"));
}

void TheMovieDBFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("superman returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheMovieDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(set(result), set(i.value().toLower()));
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QStringLiteral("Brandon Routh::Superman / Clark Kent"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void TheMovieDBFetcherTest::testTitleFr() {
  KConfig config(QFINDTESTDATA("tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QStringLiteral("TMDB FR");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("superman returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheMovieDBFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QStringList fields = QStringList() << QStringLiteral("title")
                                     << QStringLiteral("studio")
                                     << QStringLiteral("year")
                                     << QStringLiteral("title");
  foreach(const QString& field, fields) {
    QString result = entry->field(field).toLower();
    QCOMPARE(set(result), set(m_fieldValues.value(field).toLower()));
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QStringLiteral("Brandon Routh::Superman / Clark Kent"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

// see https://bugs.kde.org/show_bug.cgi?id=336765
void TheMovieDBFetcherTest::testBabel() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("babel"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheMovieDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("title"), QStringLiteral("Babel"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(set(entry, "director"), set(QString::fromUtf8("Alejandro González Iñárritu")));
  QCOMPARE(set(entry, "producer"), set(QString::fromUtf8("Alejandro González Iñárritu; Steve Golin; Jon Kilik; Ann Ruark; Corinne Golden Weber")));
}
