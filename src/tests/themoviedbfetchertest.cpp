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

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( TheMovieDBFetcherTest )

TheMovieDBFetcherTest::TheMovieDBFetcherTest() : AbstractFetcherTest() {
}

void TheMovieDBFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("Superman Returns"));
  m_fieldValues.insert(QStringLiteral("studio"), QStringLiteral("Warner Bros. Pictures; Bad Hat Harry Productions; "
                                                                "Peters Entertainment; DC Comics; Legendary Pictures"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("2006"));
  m_fieldValues.insert(QStringLiteral("genre"), QStringLiteral("action; adventure; science fiction"));
  m_fieldValues.insert(QStringLiteral("director"), QStringLiteral("Bryan Singer"));
  m_fieldValues.insert(QStringLiteral("producer"), QStringLiteral("Bryan Singer; Jon Peters; Gilbert Adler"));
//  m_fieldValues.insert(QStringLiteral("running-time"), QStringLiteral("154"));
  m_fieldValues.insert(QStringLiteral("nationality"), QStringLiteral("USA"));
}

void TheMovieDBFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("superman returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheMovieDBFetcher(this));

  // want the 2006 movie
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 2);
  Tellico::Data::EntryPtr entry;
  for(const auto& testEntry : qAsConst(results)) {
    if(testEntry->field("year") == QLatin1String("2006")) {
      entry = testEntry;
      break;
    }
  }
  QVERIFY(entry);

  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(set(result), set(i.value().toLower()));
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QStringLiteral("Brandon Routh::Clark Kent / Superman"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void TheMovieDBFetcherTest::testTitleFr() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("TMDB FR"));
  cg.writeEntry("Locale", QStringLiteral("fr"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("superman returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheMovieDBFetcher(this));
  fetcher->readConfig(cg);

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
  QCOMPARE(castList.at(0), QStringLiteral("Brandon Routh::Clark Kent / Superman"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

// see https://bugs.kde.org/show_bug.cgi?id=336765
void TheMovieDBFetcherTest::testBabel() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("babel"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheMovieDBFetcher(this));

  // want the 2006 movie
  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 2);
  Tellico::Data::EntryPtr entry;
  for(const auto& testEntry : qAsConst(results)) {
    if(testEntry->field("year") == QLatin1String("2006")) {
      entry = testEntry;
      break;
    }
  }
  QVERIFY(entry);

  QCOMPARE(entry->field("title"), QStringLiteral("Babel"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(set(entry, "director"), set(QString::fromUtf8("Alejandro González Iñárritu")));
  QCOMPARE(set(entry, "producer"), set(QString::fromUtf8("Alejandro González Iñárritu; Steve Golin; Jon Kilik; Ann Ruark; Corinne Golden Weber")));
}

void TheMovieDBFetcherTest::testAllMankind() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("TMDB"));
  cg.writeEntry("Custom Fields", QStringLiteral("origtitle,alttitle,network,episode,tmdb,imdb"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QStringLiteral("for all mankind"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheMovieDBFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field("title"), QStringLiteral("For All Mankind"));
  QCOMPARE(entry->field("origtitle"), QStringLiteral("For All Mankind"));
  QStringList titleList = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("alttitle")));
  QVERIFY(!titleList.isEmpty());
  QVERIFY(titleList.contains(QStringLiteral("为全人类")));
  QCOMPARE(entry->field("year"), QStringLiteral("2019"));
  QCOMPARE(entry->field("network"), QStringLiteral("Apple TV+"));
  QCOMPARE(entry->field("language"), QStringLiteral("English"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("USA"));
  QCOMPARE(set(entry, "producer"), set(QStringLiteral("Huey M. Park")));
  QVERIFY(entry->field("cast").startsWith(QStringLiteral("Joel Kinnaman::Ed Baldwin")));
  QStringList episodeList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("episode")));
  QVERIFY(!episodeList.isEmpty());
  QCOMPARE(episodeList.at(0), QStringLiteral("Red Moon::1::1"));
  // imdb should be empty
  QVERIFY(entry->field(QStringLiteral("imdb")).isEmpty());
  QCOMPARE(entry->field(QStringLiteral("tmdb")), QStringLiteral("https://www.themoviedb.org/tv/87917"));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QVERIFY(!entry->field(QStringLiteral("plot")).isEmpty());
}

void TheMovieDBFetcherTest::testUpdate() {
  Tellico::Data::CollPtr coll(new Tellico::Data::VideoCollection(true));
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("imdb"),
                                                         QStringLiteral("IMDB"),
                                                         Tellico::Data::Field::URL));
  QCOMPARE(coll->addField(field), true);
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  coll->addEntries(entry);
  entry->setField(QStringLiteral("title"), QStringLiteral("The Man From Snowy River"));
  entry->setField(QStringLiteral("year"), QStringLiteral("1982"));

  Tellico::Fetch::TheMovieDBFetcher fetcher(this);
  auto request = fetcher.updateRequest(entry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QVERIFY(request.value().contains(QStringLiteral("year=1982")));

  entry->setField(QStringLiteral("imdb"), QStringLiteral("https://www.imdb.com/title/tt0084296/?ref_=nv_sr_srsg_0"));
  request = fetcher.updateRequest(entry);
  QCOMPARE(request.key(), Tellico::Fetch::Raw);
  QCOMPARE(request.value(), QStringLiteral("external_source=imdb_id"));
  QCOMPARE(request.data(), QStringLiteral("/find/tt0084296"));
}
