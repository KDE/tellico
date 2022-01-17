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

#include <QTest>

QTEST_MAIN( CsvTest )

#define QL1(x) QStringLiteral(x)

void CsvTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
}

void CsvTest::cleanupTestCase() {
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
    << (QStringList() << QL1("robby") << QL1("stephenson is cool") << QString() << QString());
  QTest::newRow("space") << "robby,stephenson is cool\t,," << " "
    << (QStringList() << QL1("robby,stephenson") << QL1("is") << QL1("cool\t,,"));
  QTest::newRow("tab") << "robby\t\tstephenson" << "\t"
    << (QStringList() << QL1("robby") << QString() << QL1("stephenson"));
  // quotes get swallowed
  QTest::newRow("quotes") << "robby,\"stephenson,is,cool\"" << ","
    << (QStringList() << QL1("robby") << QL1("stephenson,is,cool"));
  QTest::newRow("newline") << "robby,\"stephenson\n,is,cool\"" << ","
    << (QStringList() << QL1("robby") << QL1("stephenson\n,is,cool"));
}

void CsvTest::testEntry() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true));
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField(QStringLiteral("title"), QStringLiteral("title, with comma"));
  coll->addEntries(entry);

  Tellico::Export::CSVExporter exporter(coll);
  exporter.setEntries(coll->entries());
  exporter.setFields(Tellico::Data::FieldList() << coll->fieldByName(QStringLiteral("title")));

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
  importer.setImportColumns({0, 1, 3},
                            {QStringLiteral("title"), QStringLiteral("author"), QStringLiteral("isbn")});
  importer.slotFirstRowHeader(true);
  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);

  Tellico::Data::EntryList entries = coll->entries();
  QCOMPARE(entries.size(), 1);
  Tellico::Data::EntryPtr entry = entries.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Cruel as the Grave"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Penman, Sharon"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0140270760"));
}
