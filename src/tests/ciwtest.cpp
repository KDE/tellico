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

#include "ciwtest.h"

#include "../translators/ciwimporter.h"
#include "../collections/bibtexcollection.h"
#include "../fieldformat.h"

#include <KLocalizedString>

#include <QTest>

QTEST_APPLESS_MAIN( CiwTest )

#define QSL(x) QStringLiteral(x)

void CiwTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
}

void CiwTest::testImport() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("/data/test.ciw"));
  QList<QUrl> urls;
  urls << url;
  Tellico::Import::CIWImporter importer(urls);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 6);
  QCOMPARE(coll->title(), QSL("Bibliography"));

  Tellico::Data::EntryPtr entry = coll->entryById(3);
  QVERIFY(entry);
  QCOMPARE(entry->field(QSL("entry-type")), QSL("article"));
  QCOMPARE(entry->field(QSL("title")), QSL("Key Process Conditions for Production of C(4) Dicarboxylic Acids in "
                                                "Bioreactor Batch Cultures of an Engineered Saccharomyces cerevisiae Strain"));
  QCOMPARE(entry->field(QSL("year")), QSL("2010"));
  QCOMPARE(entry->field(QSL("pages")), QSL("744-750"));
  QCOMPARE(entry->field(QSL("volume")), QSL("76"));
  QCOMPARE(entry->field(QSL("journal")), QSL("APPLIED AND ENVIRONMENTAL MICROBIOLOGY"));
  QCOMPARE(entry->field(QSL("doi")), QSL("10.1128/AEM.02396-09"));
  auto authors = Tellico::FieldFormat::splitValue(entry->field(QSL("author")));
  QCOMPARE(authors.count(), 5);
  QCOMPARE(authors.first(), QSL("Zelle, Rintze M."));
  QVERIFY(!entry->field(QSL("abstract")).isEmpty());

  entry = coll->entryById(6);
  QVERIFY(entry);
  QCOMPARE(entry->field(QSL("entry-type")), QSL("article"));
  QCOMPARE(entry->field(QSL("title")), QSL("Prematurity: An Overview and Public Health Implications"));
  QCOMPARE(entry->field(QSL("booktitle")), QSL("ANNUAL REVIEW OF PUBLIC HEALTH, VOL 32"));
  QCOMPARE(entry->field(QSL("isbn")), QSL("978-0-8243-2732-3"));
  authors = Tellico::FieldFormat::splitValue(entry->field(QSL("author")));
  QCOMPARE(authors.count(), 4);
  QCOMPARE(authors.first(), QSL("McCormick, Marie C."));
  const auto editors = Tellico::FieldFormat::splitValue(entry->field(QSL("editor")));
  QCOMPARE(editors.count(), 3);
  QCOMPARE(editors.first(), QSL("Fielding, JE"));

  Tellico::Data::BibtexCollection* bColl = dynamic_cast<Tellico::Data::BibtexCollection*>(coll.data());
  QVERIFY(bColl);
  QCOMPARE(bColl->fieldByBibtexName(QSL("entry-type"))->name(), QSL("entry-type"));
  QVERIFY(Tellico::Import::CIWImporter::maybeCIW(url));
}
