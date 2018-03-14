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

#include "amazonfetchertest.h"

#include "../fetch/amazonfetcher.h"
#include "../collections/bookcollection.h"
#include "../collections/musiccollection.h"
#include "../collections/videocollection.h"
#include "../collections/gamecollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KConfigGroup>

#include <QTest>

QTEST_GUILESS_MAIN( AmazonFetcherTest )

AmazonFetcherTest::AmazonFetcherTest() : AbstractFetcherTest(), m_hasConfigFile(false)
    , m_config(QFINDTESTDATA("tellicotest_private.config"), KConfig::SimpleConfig) {
}

void AmazonFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "music");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "mvideo");
  Tellico::RegisterCollection<Tellico::Data::GameCollection> registerGame(Tellico::Data::Collection::Game, "game");
  // since we use an XSL file
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/amazon2tellico.xsl"));
  Tellico::ImageFactory::init();

  m_hasConfigFile = QFile::exists(QFINDTESTDATA("tellicotest_private.config"));

  QHash<QString, QString> practicalRdf;
  practicalRdf.insert(QStringLiteral("title"), QStringLiteral("Practical RDF"));
  practicalRdf.insert(QStringLiteral("isbn"), QStringLiteral("0-596-00263-7"));
  practicalRdf.insert(QStringLiteral("author"), QStringLiteral("Shelley Powers"));
  practicalRdf.insert(QStringLiteral("binding"), QStringLiteral("Paperback"));
  practicalRdf.insert(QStringLiteral("publisher"), QStringLiteral("O'Reilly Media"));
  practicalRdf.insert(QStringLiteral("pages"), QStringLiteral("331"));

  QHash<QString, QString> gloryRevealed;
  gloryRevealed.insert(QStringLiteral("title"), QStringLiteral("Glory Revealed II"));
  gloryRevealed.insert(QStringLiteral("medium"), QStringLiteral("Compact Disc"));
//  gloryRevealed.insert(QStringLiteral("artist"), QStringLiteral("Various Artists"));
  gloryRevealed.insert(QStringLiteral("label"), QStringLiteral("Reunion"));
  gloryRevealed.insert(QStringLiteral("year"), QStringLiteral("2009"));

  QHash<QString, QString> incredibles;
  incredibles.insert(QStringLiteral("title"), QStringLiteral("Incredibles"));
  incredibles.insert(QStringLiteral("medium"), QStringLiteral("DVD"));
//  incredibles.insert(QStringLiteral("certification"), QStringLiteral("PG (USA)"));
//  incredibles.insert(QStringLiteral("studio"), QStringLiteral("Walt Disney Home Entertainment"));
//  incredibles.insert(QStringLiteral("year"), QStringLiteral("2004"));
  incredibles.insert(QStringLiteral("widescreen"), QStringLiteral("true"));
  incredibles.insert(QStringLiteral("director"), QStringLiteral("Brad Bird; Bud Luckey; Roger Gould"));

  QHash<QString, QString> pacteDesLoups;
  pacteDesLoups.insert(QStringLiteral("title"), QStringLiteral("Le Pacte des Loups"));
  pacteDesLoups.insert(QStringLiteral("medium"), QStringLiteral("Blu-ray"));
//  pacteDesLoups.insert(QStringLiteral("region"), QStringLiteral("Region 2"));
  pacteDesLoups.insert(QStringLiteral("studio"), QStringLiteral("StudioCanal"));
  pacteDesLoups.insert(QStringLiteral("year"), QStringLiteral("2001"));
  pacteDesLoups.insert(QStringLiteral("director"), QStringLiteral("Christophe Gans"));
//  pacteDesLoups.insert(QStringLiteral("format"), QStringLiteral("PAL"));

  QHash<QString, QString> petitPrinceCN;
  petitPrinceCN.insert(QStringLiteral("title"), QStringLiteral("小王子(65周年纪念版)"));
  petitPrinceCN.insert(QStringLiteral("author"), QStringLiteral("圣埃克絮佩里 (Saint-Exupery)"));

  m_fieldValues.insert(QStringLiteral("practicalRdf"), practicalRdf);
  m_fieldValues.insert(QStringLiteral("gloryRevealed"), gloryRevealed);
  m_fieldValues.insert(QStringLiteral("incredibles"), incredibles);
  m_fieldValues.insert(QStringLiteral("pacteDesLoups"), pacteDesLoups);
  m_fieldValues.insert(QStringLiteral("petitPrinceCN"), petitPrinceCN);
}

void AmazonFetcherTest::testTitle() {
  QFETCH(QString, locale);
  QFETCH(int, collType);
  QFETCH(QString, searchValue);
  QFETCH(QString, resultName);

  QString groupName = QStringLiteral("Amazon ") + locale;
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Amazon settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(collType, Tellico::Fetch::Title, searchValue);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AmazonFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues.value(resultName));
  while(i.hasNext()) {
    i.next();
    // a known bug is CA video titles result in music results, so only title matches
    if(i.key() != QStringLiteral("title")) {
      QEXPECT_FAIL("CA video title", "CA video titles show music results for some reason", Continue);
    }
    QString result = entry->field(i.key()).toLower();
    // several titles have edition info in the title
    if(collType == Tellico::Data::Collection::Video &&
       i.key() == QStringLiteral("title") &&
       (locale == QStringLiteral("CA") ||
        locale == QStringLiteral("FR") ||
        locale == QStringLiteral("ES") ||
        locale == QStringLiteral("CN") ||
        locale == QStringLiteral("IT") ||
        locale == QStringLiteral("DE"))) {
      QVERIFY2(result.contains(i.value(), Qt::CaseInsensitive), qPrintable(i.key()));
    } else if(collType == Tellico::Data::Collection::Video &&
       i.key() == QStringLiteral("year") &&
       locale == QStringLiteral("FR")) {
      // france has no year for movie
      QCOMPARE(result, QString());
    } else if(collType == Tellico::Data::Collection::Video &&
              i.key() == QStringLiteral("medium") &&
              (locale == QStringLiteral("ES") || locale == QStringLiteral("IT"))) {
      // ES and IT think it's a DVD
      QCOMPARE(result, QStringLiteral("dvd"));
    } else if(i.key() == QStringLiteral("pages") &&
              (locale == QStringLiteral("UK") || locale == QStringLiteral("CA"))) {
      // UK and CA have different page count
      QCOMPARE(result, QStringLiteral("352"));
    } else if((i.key() == QStringLiteral("director") || i.key() == QStringLiteral("studio") || i.key() == QStringLiteral("year")) &&
              (locale == QStringLiteral("ES") || locale == QStringLiteral("IT"))) {
      // ES and IT have no director or studio or year info
      QCOMPARE(result, QString());
    } else {
      QCOMPARE(result, i.value().toLower());
    }
  }
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void AmazonFetcherTest::testTitle_data() {
  QTest::addColumn<QString>("locale");
  QTest::addColumn<int>("collType");
  QTest::addColumn<QString>("searchValue");
  QTest::addColumn<QString>("resultName");

  QTest::newRow("US book title") << QStringLiteral("US")
                                 << static_cast<int>(Tellico::Data::Collection::Book)
                                 << QStringLiteral("Practical RDF")
                                 << QStringLiteral("practicalRdf");
  QTest::newRow("UK book title") << QStringLiteral("UK")
                                 << static_cast<int>(Tellico::Data::Collection::Book)
                                 << QStringLiteral("Practical RDF")
                                 << QStringLiteral("practicalRdf");
//  QTest::newRow("DE") << QString::fromLatin1("DE");
//  QTest::newRow("JP") << QString::fromLatin1("JP");
//  QTest::newRow("FR") << QString::fromLatin1("FR");
  QTest::newRow("CA book title") << QStringLiteral("CA")
                                 << static_cast<int>(Tellico::Data::Collection::Book)
                                 << QStringLiteral("Practical RDF")
                                 << QStringLiteral("practicalRdf");
  QTest::newRow("CN book title") << QStringLiteral("CN")
                                  << static_cast<int>(Tellico::Data::Collection::Book)
                                  << QStringLiteral("小王子(65周年纪念版)")
                                  << QStringLiteral("petitPrinceCN");

  // a known bug is CA video titles result in music results, so only title matches
//  QTest::newRow("CA video title") << QString::fromLatin1("CA")
//                                  << static_cast<int>(Tellico::Data::Collection::Video)
//                                  << QString::fromLatin1("Le Pacte des Loups")
//                                  << QString::fromLatin1("pacteDesLoups");
  QTest::newRow("FR video title") << QStringLiteral("FR")
                                  << static_cast<int>(Tellico::Data::Collection::Video)
                                  << QStringLiteral("Le Pacte des Loups")
                                  << QStringLiteral("pacteDesLoups");
  QTest::newRow("ES video title") << QStringLiteral("ES")
                                  << static_cast<int>(Tellico::Data::Collection::Video)
                                  << QStringLiteral("Le Pacte des Loups")
                                  << QStringLiteral("pacteDesLoups");
  QTest::newRow("IT video title") << QStringLiteral("IT")
                                  << static_cast<int>(Tellico::Data::Collection::Video)
                                  << QStringLiteral("Le Pacte des Loups")
                                  << QStringLiteral("pacteDesLoups");

}

void AmazonFetcherTest::testTitleVideoGame() {
  QString groupName = QStringLiteral("Amazon US");
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Amazon settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Title,
                                       QStringLiteral("Ghostbusters Story Pack - LEGO Dimensions"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AmazonFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Ghostbusters Story Pack - LEGO Dimensions"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Warner Home Video - Games"));
  // the E10+ ESRB rating was added to Tellico in 2017 in version 3.0.1
  QCOMPARE(entry->field("certification"), QStringLiteral("Everyone 10+"));
}

void AmazonFetcherTest::testIsbn() {
  QFETCH(QString, locale);
  QFETCH(QString, searchValue);
  QFETCH(QString, resultName);

  QString groupName = QStringLiteral("Amazon ") + locale;
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Amazon settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  // also testing multiple values
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       searchValue);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AmazonFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QCOMPARE(results.size(), 2);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues.value(resultName));
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    if(i.key() == QStringLiteral("pages") &&
       (locale == QStringLiteral("UK") || locale == QStringLiteral("CA"))) {
      // UK and CA have different page count
      QCOMPARE(result, QStringLiteral("352"));
    } else {
      QCOMPARE(result, i.value().toLower());
    }
  }
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}

void AmazonFetcherTest::testIsbn_data() {
  QTest::addColumn<QString>("locale");
  QTest::addColumn<QString>("searchValue");
  QTest::addColumn<QString>("resultName");

  QTest::newRow("US isbn") << QStringLiteral("US")
                           << QStringLiteral("0-596-00263-7; 978-1-59059-831-3")
                           << QStringLiteral("practicalRdf");
  QTest::newRow("UK isbn") << QStringLiteral("UK")
                           << QStringLiteral("0-596-00263-7; 978-1-59059-831-3")
                           << QStringLiteral("practicalRdf");
//  QTest::newRow("DE") << QString::fromLatin1("DE");
//  QTest::newRow("JP") << QString::fromLatin1("JP");
//  QTest::newRow("FR") << QString::fromLatin1("FR");
  QTest::newRow("CA isbn") << QStringLiteral("CA")
                           << QStringLiteral("0-596-00263-7; 978-1-59059-831-3")
                           << QStringLiteral("practicalRdf");
/*
  QTest::newRow("CN isbn") << QString::fromLatin1("CN")
                           << QString::fromLatin1("7511305202")
                           << QString::fromLatin1("petitPrinceCN");
*/
}

void AmazonFetcherTest::testUpc() {
  QFETCH(QString, locale);
  QFETCH(int, collType);
  QFETCH(QString, searchValue);
  QFETCH(QString, resultName);

  QString groupName = QStringLiteral("Amazon ") + locale;
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Amazon settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(collType, Tellico::Fetch::UPC, searchValue);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AmazonFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QVERIFY(!results.isEmpty());

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues.value(resultName));
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    // exception for UK label different than US
    // and FR title having edition info
    if((i.key() == QStringLiteral("label") && locale == QStringLiteral("UK")) ||
       (i.key() == QStringLiteral("title"))) {
      QVERIFY(result.contains(i.value(), Qt::CaseInsensitive));
    } else if(i.key() == QStringLiteral("year") &&
       locale == QStringLiteral("FR")) {
      // france has no year for movie
      QCOMPARE(result, QString());
    } else {
      QCOMPARE(result, i.value().toLower());
    }
  }
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  if(collType == Tellico::Data::Collection::Album) {
    QVERIFY(!entry->field(QStringLiteral("genre")).isEmpty());
    QVERIFY(!entry->field(QStringLiteral("track")).isEmpty());
  } else if(collType == Tellico::Data::Collection::Video) {
    QVERIFY(!entry->field(QStringLiteral("cast")).isEmpty());
  }
}

void AmazonFetcherTest::testUpc_data() {
  QTest::addColumn<QString>("locale");
  QTest::addColumn<int>("collType");
  QTest::addColumn<QString>("searchValue");
  QTest::addColumn<QString>("resultName");

  QTest::newRow("US music upc") << QStringLiteral("US")
                                << static_cast<int>(Tellico::Data::Collection::Album)
                                << QStringLiteral("602341013727")
                                << QStringLiteral("gloryRevealed");
  // non-US should work with or without the initial 0 country code
  QTest::newRow("UK music upc1") << QStringLiteral("UK")
                                << static_cast<int>(Tellico::Data::Collection::Album)
                                << QStringLiteral("602341013727")
                                << QStringLiteral("gloryRevealed");
  QTest::newRow("UK music upc2") << QStringLiteral("UK")
                                << static_cast<int>(Tellico::Data::Collection::Album)
                                << QStringLiteral("0602341013727")
                                << QStringLiteral("gloryRevealed");
  QTest::newRow("CA music upc") << QStringLiteral("CA")
                                << static_cast<int>(Tellico::Data::Collection::Album)
                                << QStringLiteral("0602341013727")
                                << QStringLiteral("gloryRevealed");

  QTest::newRow("US movie upc") << QStringLiteral("US")
                                << static_cast<int>(Tellico::Data::Collection::Video)
                                << QStringLiteral("786936244250")
                                << QStringLiteral("incredibles");
  QTest::newRow("UK movie upc") << QStringLiteral("UK")
                                << static_cast<int>(Tellico::Data::Collection::Video)
                                << QStringLiteral("0786936244250")
                                << QStringLiteral("incredibles");
  QTest::newRow("CA movie upc") << QStringLiteral("CA")
                                << static_cast<int>(Tellico::Data::Collection::Video)
                                << QStringLiteral("0786936244250")
                                << QStringLiteral("incredibles");
  QTest::newRow("FR movie upc") << QStringLiteral("FR")
                                << static_cast<int>(Tellico::Data::Collection::Video)
                                << QStringLiteral("5050582560985")
                                << QStringLiteral("pacteDesLoups");
}
