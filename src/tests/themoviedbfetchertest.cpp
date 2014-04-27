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
#include "themoviedbfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/themoviedbfetcher.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KStandardDirs>
#include <KConfigGroup>

QTEST_KDEMAIN( TheMovieDBFetcherTest, GUI )

TheMovieDBFetcherTest::TheMovieDBFetcherTest() : AbstractFetcherTest() {
}

void TheMovieDBFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  // since we use the importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QLatin1String("title"), QLatin1String("Superman Returns"));
  m_fieldValues.insert(QLatin1String("studio"), QLatin1String("Warner Bros. Pictures; Dark Castle"));
  m_fieldValues.insert(QLatin1String("year"), QLatin1String("2006"));
  m_fieldValues.insert(QLatin1String("genre"), QLatin1String("action; adventure; fantasy; science fiction"));
  m_fieldValues.insert(QLatin1String("director"), QLatin1String("Bryan Singer"));
  m_fieldValues.insert(QLatin1String("producer"), QLatin1String("Bryan Singer"));
  m_fieldValues.insert(QLatin1String("running-time"), QLatin1String("154"));
  m_fieldValues.insert(QLatin1String("nationality"), QLatin1String("USA"));
}

void TheMovieDBFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QLatin1String("superman returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheMovieDBFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    //    QVERIFY(result.contains(i.value().toLower()));
    QCOMPARE(result, i.value().toLower());
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QLatin1String("Brandon Routh::Superman / Clark Kent"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
}

void TheMovieDBFetcherTest::testTitleFr() {
  KConfig config(QString::fromLatin1(KDESRCDIR)  + "/tellicotest.config", KConfig::SimpleConfig);
  QString groupName = QLatin1String("TMDB FR");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title,
                                       QLatin1String("superman returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::TheMovieDBFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QStringList fields = QStringList() << QLatin1String("title")
                                     << QLatin1String("studio")
                                     << QLatin1String("year")
                                     << QLatin1String("title");
  foreach(const QString& field, fields) {
    QString result = entry->field(field).toLower();
    QCOMPARE(result, m_fieldValues.value(field).toLower());
  }
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field("cast"));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QLatin1String("Brandon Routh::Superman / Clark Kent"));
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("plot")).isEmpty());
}

