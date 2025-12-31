/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "csvtest.h"

#include "../translators/csvparser.h"
#include "../translators/csvimporter.h"
#include "../translators/csvexporter.h"
#include "../collectionfactory.h"
#include "../collections/bookcollection.h"
#include "../collections/musiccollection.h"
#include "../images/imagefactory.h"

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>
#include <QStandardPaths>

QTEST_MAIN( CsvTest )

#define QSL(x) QStringLiteral(x)

void CsvTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerAlbum(Tellico::Data::Collection::Album, "album");
}

void CsvTest::init() {
  // since the importer saves the settings, clear them out
  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ImportOptions - CSV"));
  config.deleteGroup();
}

void CsvTest::testTokens() {
  QFETCH(QString, line);
  QFETCH(QStringList, tokens);
  QFETCH(QString, delim);

  Tellico::CSVParser p(line);
  p.setDelimiter(delim);

  QStringList tokensNew = p.nextTokens();

  QCOMPARE(tokensNew, tokens);
}

void CsvTest::testTokens_data() {
  QTest::addColumn<QString>("line");
  QTest::addColumn<QString>("delim");
  QTest::addColumn<QStringList>("tokens");

  QTest::newRow("basic") << "robby,stephenson is cool\t,," << ","
    << (QStringList() << QSL("robby") << QSL("stephenson is cool") << QString() << QString());
  QTest::newRow("space") << "robby,stephenson is cool\t,," << " "
    << (QStringList() << QSL("robby,stephenson") << QSL("is") << QSL("cool\t,,"));
  QTest::newRow("tab") << "robby\t\tstephenson" << "\t"
    << (QStringList() << QSL("robby") << QString() << QSL("stephenson"));
  // quotes get swallowed
  QTest::newRow("quotes") << "robby,\"stephenson,is,cool\"" << ","
    << (QStringList() << QSL("robby") << QSL("stephenson,is,cool"));
  QTest::newRow("newline") << "robby,\"stephenson\n,is,cool\"" << ","
    << (QStringList() << QSL("robby") << QSL("stephenson\n,is,cool"));
}

void CsvTest::testEntry() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true));
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField(QStringLiteral("title"), QStringLiteral("title, with comma"));
  coll->addEntries(entry);

  Tellico::Export::CSVExporter exporter(coll);
  exporter.setEntries(coll->entries());
  exporter.setFields(Tellico::Data::FieldList() << coll->fieldByName(QStringLiteral("title")));

  QCOMPARE(exporter.formatString(), QLatin1String("CSV"));
  QVERIFY(exporter.fileFilter().contains(QLatin1String(";;All Files (*)")));

  QString output = exporter.text();
  // the header line has the field titles, skip that
  output = output.section(QLatin1Char('\n'), 1);
  output.chop(1);
  QCOMPARE(output, QStringLiteral("\"title, with comma\""));
}

void CsvTest::testImportBook() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-book.csv"));
  Tellico::Import::CSVImporter importer(url);
  importer.setCollectionType(Tellico::Data::Collection::Book);
  importer.setImportColumns({0, 1, 3, 4, 5},
                            {QSL("title"),
                             QSL("author"),
                             QSL("isbn"),
                             QSL("binding"),
                             QSL("cover")});
  importer.slotFirstRowHeader(true);
  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);

  Tellico::Data::EntryList entries = coll->entries();
  QCOMPARE(entries.size(), 1);
  Tellico::Data::EntryPtr entry = entries.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QSL("title")), QSL("Cruel as the Grave"));
  QCOMPARE(entry->field(QSL("author")), QSL("Penman, Sharon"));
  QCOMPARE(entry->field(QSL("isbn")), QSL("0140270760"));
  QCOMPARE(entry->field(QSL("binding")), QSL("square"));
  // can't use the actual md5 for some reason, its calculated differently in the CI
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QSL("collectorz")));
}

void CsvTest::testBug386483() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-bug386483.csv"));
  Tellico::Import::CSVImporter importer(url);
  importer.setCollectionType(Tellico::Data::Collection::Album);
  importer.setImportColumns({0, 1, 2, 3, 6},
                            {QSL("title"),
                             QSL("label"),
                             QSL("year"),
                             QSL("track"),
                             QSL("keyword")});
  importer.slotFirstRowHeader(false);
  importer.setDelimiter(QStringLiteral(","));
  importer.setColumnDelimiter(QStringLiteral("::"));
  importer.setRowDelimiter(QStringLiteral("|"));
  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);

  Tellico::Data::EntryList entries = coll->entries();
  QCOMPARE(entries.size(), 1);
  Tellico::Data::EntryPtr entry = entries.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("album"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("2022"));
  QCOMPARE(entry->field(QStringLiteral("keyword")), QStringLiteral("https://tellico-project.org"));
  QStringList trackList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("track")));
  QCOMPARE(trackList.size(), 2);
  QStringList cols = Tellico::FieldFormat::splitRow(trackList.at(1));
  QCOMPARE(cols.size(), 3);
  QCOMPARE(cols.at(0), QStringLiteral("track2"));
  QCOMPARE(cols.at(1), QStringLiteral("artist2"));
  QCOMPARE(cols.at(2), QStringLiteral("0:34"));
}

void CsvTest::testDateFormat() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-date.csv"));
  Tellico::Import::CSVImporter importer(url);
  importer.setCollectionType(Tellico::Data::Collection::Book);
  importer.setImportColumns({0, 1, 2},
                            {QSL("title"),
                             QSL("cdate"),
                             QSL("pur_date")});
  importer.slotFirstRowHeader(false);
  importer.setDelimiter(QStringLiteral(","));
  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);

  Tellico::Data::EntryList entries = coll->entries();
  QCOMPARE(entries.size(), 2);
  Tellico::Data::EntryPtr entry = entries.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("title1"));
  QCOMPARE(entry->field(QStringLiteral("cdate")), QStringLiteral("2025-12-25"));
  QCOMPARE(entry->field(QStringLiteral("pur_date")), QStringLiteral("2025-12-25"));
  entry = entries.at(1);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("title2"));
  QCOMPARE(entry->field(QStringLiteral("cdate")), QStringLiteral("2025-12-25"));
  QCOMPARE(entry->field(QStringLiteral("pur_date")), QStringLiteral("2025-12-25"));
}
