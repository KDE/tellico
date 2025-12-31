/***************************************************************************
    Copyright (C) 2009-2023 Robby Stephenson <robby@periapsis.org>
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

#include "entrycomparisontest.h"

#include "../collection.h"
#include "../field.h"
#include "../entry.h"
#include "../collections/bookcollection.h"
#include "../collections/bibtexcollection.h"
#include "../collections/videocollection.h"
#include "../collections/comicbookcollection.h"
#include "../collections/musiccollection.h"
#include "../collections/gamecollection.h"
#include "../collections/filecatalog.h"
#include "../entrycomparison.h"

#include <KLocalizedString>

#include <QTest>
#include <QLoggingCategory>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( EntryComparisonTest )

Q_DECLARE_METATYPE(Tellico::EntryComparison::MatchValue)

void EntryComparisonTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QLoggingCategory::setFilterRules(QStringLiteral("tellico.debug = true\ntellico.info = false"));
  qRegisterMetaType<Tellico::EntryComparison::MatchValue>();
  KLocalizedString::setApplicationDomain("tellico");
//  Tellico::ImageFactory::init();

  // create the collection and entry used for testMatchScore()
  m_coll = Tellico::Data::CollPtr(new Tellico::Data::BookCollection(true));
  m_entry = Tellico::Data::EntryPtr(new Tellico::Data::Entry(m_coll));
  m_entry->setField(QStringLiteral("title"), QStringLiteral("title1"));
  m_entry->setField(QStringLiteral("author"), QStringLiteral("John Doe"));
  m_entry->setField(QStringLiteral("isbn"), QStringLiteral("9791234567896"));
  m_entry->setField(QStringLiteral("lccn"), QStringLiteral("89456"));
  Tellico::Data::FieldPtr f(new Tellico::Data::Field(QStringLiteral("arxiv"), QStringLiteral("Arxiv ID")));
  m_coll->addField(f);
  m_entry->setField(QStringLiteral("arxiv"), QStringLiteral("hep-lat/0110180"));
  m_coll->addEntries(m_entry);
}

void EntryComparisonTest::cleanupTestCase() {
//  Tellico::ImageFactory::clean(true);
}

void EntryComparisonTest::testMatchScore() {
  QFETCH(QString, field);
  QFETCH(QString, value);
  QFETCH(Tellico::EntryComparison::MatchValue, score);

  QVERIFY(m_coll);
  Tellico::Data::EntryPtr e(new Tellico::Data::Entry(m_coll));
  e->setField(field, value);
  QCOMPARE(Tellico::EntryComparison::score(m_entry, e, field, m_coll.data()), int(score));
}

void EntryComparisonTest::testMatchScore_data() {
  QTest::addColumn<QString>("field");
  QTest::addColumn<QString>("value");
  QTest::addColumn<Tellico::EntryComparison::MatchValue>("score");

  QTest::newRow("empty title") << QStringLiteral("title") << QString() << Tellico::EntryComparison::MATCH_VALUE_NONE;
  QTest::newRow("title match") << QStringLiteral("title") << QStringLiteral("title1") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("title match case") << QStringLiteral("title") << QStringLiteral("TITLE1") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
//  QTest::newRow("title match articles") << QStringLiteral("title") << QStringLiteral("THE TITLE1") << Tellico::EntryComparison::MATCH_VALUE_WEAK;
  QTest::newRow("title match non alphanum") << QStringLiteral("title") << QStringLiteral("title1.") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
//  QTest::newRow("title match paren") << QStringLiteral("title") << QStringLiteral("title1 (old)") << Tellico::EntryComparison::MATCH_VALUE_WEAK;
  QTest::newRow("isbn match") << QStringLiteral("isbn") << QStringLiteral("9791234567896") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("isbn match formatted") << QStringLiteral("isbn") << QStringLiteral("979-1-23456-789-6") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("lccn match") << QStringLiteral("lccn") << QStringLiteral("89456") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("lccn match formatted") << QStringLiteral("lccn") << QStringLiteral("89-456") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("arxiv") << QStringLiteral("arxiv") << QStringLiteral("hep-lat/0110180") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("arxiv format1") << QStringLiteral("arxiv") << QStringLiteral("hep-lat/0110180v1") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("arxiv format2") << QStringLiteral("arxiv") << QStringLiteral("arxiv:hep-lat/0110180v1") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("author") << QStringLiteral("author") << QStringLiteral("John Doe") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("author formatted") << QStringLiteral("author") << QStringLiteral("Doe, John") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("author formatted2") << QStringLiteral("author") << QStringLiteral("doe, john") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
  QTest::newRow("author multiple") << QStringLiteral("author") << QStringLiteral("John Doe; Jane Doe") << Tellico::EntryComparison::MATCH_VALUE_STRONG;
}

void EntryComparisonTest::testBookMatch() {
  Tellico::Data::CollPtr c(new Tellico::Data::BookCollection(true));

  Tellico::Data::EntryPtr e1(new Tellico::Data::Entry(c));
  e1->setField(QStringLiteral("title"), QStringLiteral("title1"));
  e1->setField(QStringLiteral("author"), QStringLiteral("author1"));
  e1->setField(QStringLiteral("edition"), QStringLiteral("edition1"));
  e1->setField(QStringLiteral("pur_price"), QStringLiteral("price1"));
  e1->setField(QStringLiteral("isbn"), QStringLiteral("1234567890"));
  c->addEntries(e1);

  Tellico::Data::EntryPtr e2(new Tellico::Data::Entry(c));
  e2->setField(QStringLiteral("title"), QStringLiteral("title2"));
  e2->setField(QStringLiteral("author"), QStringLiteral("author2"));
  e2->setField(QStringLiteral("edition"), QStringLiteral("edition2"));
  e2->setField(QStringLiteral("pur_price"), QStringLiteral("price2"));
  e2->setField(QStringLiteral("isbn"), QStringLiteral("000000000"));

  // not a good match
  QVERIFY(c->sameEntry(e1, e2) < Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  // matching the title makes a good match, even if author does not match
  e2->setField(QStringLiteral("title"), QStringLiteral("title1"));
  QVERIFY(c->sameEntry(e1, e2) > Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  // title and author both matching are enough for high confidence
  e2->setField(QStringLiteral("author"), QStringLiteral("author1"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);

  // perfect match for isbn by itself
  e2->setField(QStringLiteral("title"), QString());
  e2->setField(QStringLiteral("author"), QString());
  e2->setField(QStringLiteral("isbn"), QStringLiteral("1234567890"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);
}

void EntryComparisonTest::testBibtexMatch() {
  Tellico::Data::CollPtr c(new Tellico::Data::BibtexCollection(true));

  Tellico::Data::EntryPtr e1(new Tellico::Data::Entry(c));
  e1->setField(QStringLiteral("title"), QStringLiteral("title1"));
  e1->setField(QStringLiteral("author"), QStringLiteral("author1"));
  e1->setField(QStringLiteral("edition"), QStringLiteral("edition1"));
  e1->setField(QStringLiteral("doi"), QStringLiteral("1234567890"));
  c->addEntries(e1);

  Tellico::Data::EntryPtr e2(new Tellico::Data::Entry(c));
  e2->setField(QStringLiteral("title"), QStringLiteral("title2"));
  e2->setField(QStringLiteral("author"), QStringLiteral("author2"));
  e2->setField(QStringLiteral("edition"), QStringLiteral("edition2"));
  e2->setField(QStringLiteral("doi"), QStringLiteral("000000000"));

  // not a good match
  QVERIFY(c->sameEntry(e1, e2) < Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  // matching the title makes a good match, even if author does not match
  e2->setField(QStringLiteral("title"), QStringLiteral("title1"));
  QVERIFY(c->sameEntry(e1, e2) > Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  // title and author both matching are enough for high confidence
  e2->setField(QStringLiteral("author"), QStringLiteral("author1"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);

  // perfect match for doi by itself
  e2->setField(QStringLiteral("title"), QString());
  e2->setField(QStringLiteral("author"), QString());
  e2->setField(QStringLiteral("doi"), QStringLiteral("1234567890"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);
}

void EntryComparisonTest::testComicMatch() {
  Tellico::Data::CollPtr c(new Tellico::Data::ComicBookCollection(true));

  Tellico::Data::EntryPtr e1(new Tellico::Data::Entry(c));
  e1->setField(QStringLiteral("title"), QStringLiteral("title1"));
  e1->setField(QStringLiteral("series"), QStringLiteral("series1"));
  e1->setField(QStringLiteral("issue"), QStringLiteral("1"));
  c->addEntries(e1);

  Tellico::Data::EntryPtr e2(new Tellico::Data::Entry(c));
  e2->setField(QStringLiteral("title"), QStringLiteral("title2"));

  // not a good match
  QVERIFY(c->sameEntry(e1, e2) < Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  // matching the title makes a good match, even if author does not match
  e2->setField(QStringLiteral("title"), QStringLiteral("title1"));
  QVERIFY(c->sameEntry(e1, e2) > Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  // matching series and issue makes a perfect match
  e2->setField(QStringLiteral("series"), QStringLiteral("series1"));
  e2->setField(QStringLiteral("issue"), QStringLiteral("1"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);
}

void EntryComparisonTest::testVideoMatch() {
  Tellico::Data::CollPtr c(new Tellico::Data::VideoCollection(true));
  Tellico::Data::FieldPtr f(new Tellico::Data::Field(QStringLiteral("imdb"),
                                                     QStringLiteral("IMDB"),
                                                     Tellico::Data::Field::URL));
  c->addField(f);

  Tellico::Data::EntryPtr e1(new Tellico::Data::Entry(c));
  e1->setField(QStringLiteral("title"), QStringLiteral("the man from snowy river"));
  e1->setField(QStringLiteral("year"), QStringLiteral("1982"));
  e1->setField(QStringLiteral("director"), QStringLiteral("a director"));
  e1->setField(QStringLiteral("studio"), QStringLiteral("studio 7"));
  c->addEntries(e1);

  Tellico::Data::EntryPtr e2(new Tellico::Data::Entry(c));
  e2->setField(QStringLiteral("title"), QStringLiteral("man from snowy river, the"));

  // not a good match
  QVERIFY(c->sameEntry(e1, e2) < Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  // matching the year makes a good match
  e2->setField(QStringLiteral("year"), QStringLiteral("1982"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  // matching the title exactly and year makes a perfect match
  e2->setField(QStringLiteral("title"), QStringLiteral("the man from snowy river"));
  e2->setField(QStringLiteral("year"), QStringLiteral("1982"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);

  // imdb link should be perfect match by itself, ignoring host
  e1->setField(QStringLiteral("imdb"), QStringLiteral("https://www.imdb.com/title/tt1856080/"));
  e2->setField(QStringLiteral("imdb"), QStringLiteral("https://us.imdb.com/title/tt1856080/"));
  e2->setField(QStringLiteral("title"), QString());
  e2->setField(QStringLiteral("year"), QString());
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);
}

void EntryComparisonTest::testMusicMatch() {
  Tellico::Data::CollPtr c(new Tellico::Data::MusicCollection(true));

  Tellico::Data::EntryPtr e1(new Tellico::Data::Entry(c));
  e1->setField(QStringLiteral("title"), QStringLiteral("title1"));
  e1->setField(QStringLiteral("artist"), QStringLiteral("artist1"));
  e1->setField(QStringLiteral("label"), QStringLiteral("label"));
  e1->setField(QStringLiteral("year"), QStringLiteral("1994"));
  c->addEntries(e1);

  Tellico::Data::EntryPtr e2(new Tellico::Data::Entry(c));
  e2->setField(QStringLiteral("title"), QStringLiteral("title2"));
  e2->setField(QStringLiteral("artist"), QStringLiteral("artist2"));

  // not a good match
  QVERIFY(c->sameEntry(e1, e2) < Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  // matching the title and artist makes a good match
  e2->setField(QStringLiteral("title"), QStringLiteral("title1"));
  e2->setField(QStringLiteral("artist"), QStringLiteral("artist1"));
  QVERIFY(c->sameEntry(e1, e2) > Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  e2->setField(QStringLiteral("label"), QStringLiteral("label"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);
  e2->setField(QStringLiteral("label"), QString());
  e2->setField(QStringLiteral("year"), QStringLiteral("1994"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);
}

void EntryComparisonTest::testGameMatch() {
  Tellico::Data::CollPtr c(new Tellico::Data::GameCollection(true));

  Tellico::Data::EntryPtr e1(new Tellico::Data::Entry(c));
  e1->setField(QStringLiteral("title"), QStringLiteral("title1"));
  e1->setField(QStringLiteral("platform"), QStringLiteral("Xbox One"));
  e1->setField(QStringLiteral("publisher"), QStringLiteral("publisher"));
  e1->setField(QStringLiteral("year"), QStringLiteral("1994"));
  c->addEntries(e1);

  Tellico::Data::EntryPtr e2(new Tellico::Data::Entry(c));
  e2->setField(QStringLiteral("title"), QStringLiteral("title2"));
  e2->setField(QStringLiteral("platform"), QStringLiteral("Xbox One"));

  // not a good match
  QVERIFY(c->sameEntry(e1, e2) < Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  e2->setField(QStringLiteral("title"), QStringLiteral("title1"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  e2->setField(QStringLiteral("publisher"), QStringLiteral("publisher"));
  e2->setField(QStringLiteral("year"), QStringLiteral("1994"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);
}

void EntryComparisonTest::testFileMatch() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test.ris"));
  Tellico::Data::CollPtr c(new Tellico::Data::FileCatalog(true));

  // first check merging with same isbn
  Tellico::Data::EntryPtr e1(new Tellico::Data::Entry(c));
  e1->setField(QStringLiteral("url"), url.url());
  e1->setField(QStringLiteral("title"), QStringLiteral("file.name"));
  e1->setField(QStringLiteral("description"), QStringLiteral("desc"));
  e1->setField(QStringLiteral("mimetype"), QStringLiteral("text/plain"));
  e1->setField(QStringLiteral("size"), QStringLiteral("1234"));
  e1->setField(QStringLiteral("volume"), QStringLiteral("vol1"));
  c->addEntries(e1);

  Tellico::Data::EntryPtr e2(new Tellico::Data::Entry(c));
  e2->setField(QStringLiteral("url"), url.url());
  e2->setField(QStringLiteral("title"), QStringLiteral("file.name"));

  // perfect match by file url
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);

  e2->setField(QStringLiteral("url"), QString());
  QVERIFY(c->sameEntry(e1, e2) == Tellico::EntryComparison::ENTRY_GOOD_MATCH);

  e2->setField(QStringLiteral("description"), QStringLiteral("desc"));
  QVERIFY(c->sameEntry(e1, e2) > Tellico::EntryComparison::ENTRY_GOOD_MATCH);
  QVERIFY(c->sameEntry(e1, e2) < Tellico::EntryComparison::ENTRY_PERFECT_MATCH);
  e2->setField(QStringLiteral("mimetype"), QStringLiteral("text/plain"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);

  e2->setField(QStringLiteral("description"), QString());
  e2->setField(QStringLiteral("mimetype"), QString());
  e2->setField(QStringLiteral("size"), QStringLiteral("1234"));
  QVERIFY(c->sameEntry(e1, e2) >= Tellico::EntryComparison::ENTRY_PERFECT_MATCH);

  // if volume is different, can't be same match
  e2->setField(QStringLiteral("volume"), QStringLiteral("vol2"));
  QVERIFY(c->sameEntry(e1, e2) <= Tellico::EntryComparison::ENTRY_BAD_MATCH);
}
