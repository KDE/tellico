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

#undef QT_NO_CAST_FROM_ASCII

#include "ciwtest.h"

#include "../translators/ciwimporter.h"
#include "../collections/bibtexcollection.h"
#include "../fieldformat.h"

#include <QTest>

QTEST_APPLESS_MAIN( CiwTest )

void CiwTest::testImport() {
  KUrl url(QFINDTESTDATA("/data/test.ciw"));
  KUrl::List urls;
  urls << url;
  Tellico::Import::CIWImporter importer(urls);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 6);
  QCOMPARE(coll->title(), QLatin1String("Bibliography"));

  Tellico::Data::EntryPtr entry = coll->entryById(3);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("entry-type"), QLatin1String("article"));
  QCOMPARE(entry->field("title"), QLatin1String("Key Process Conditions for Production of C(4) Dicarboxylic Acids in "
                                                "Bioreactor Batch Cultures of an Engineered Saccharomyces cerevisiae Strain"));
  QCOMPARE(entry->field("year"), QLatin1String("2010"));
  QCOMPARE(entry->field("pages"), QLatin1String("744-750"));
  QCOMPARE(entry->field("volume"), QLatin1String("76"));
  QCOMPARE(entry->field("journal"), QLatin1String("APPLIED AND ENVIRONMENTAL MICROBIOLOGY"));
  QCOMPARE(entry->field("doi"), QLatin1String("10.1128/AEM.02396-09"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field("author")).count(), 5);
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field("author")).first(), QLatin1String("Zelle, Rintze M."));
  QVERIFY(!entry->field("abstract").isEmpty());

  entry = coll->entryById(6);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("entry-type"), QLatin1String("article"));
  QCOMPARE(entry->field("title"), QLatin1String("Prematurity: An Overview and Public Health Implications"));
  QCOMPARE(entry->field("booktitle"), QLatin1String("ANNUAL REVIEW OF PUBLIC HEALTH, VOL 32"));
  QCOMPARE(entry->field("isbn"), QLatin1String("978-0-8243-2732-3"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field("author")).count(), 4);
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field("author")).first(), QLatin1String("McCormick, Marie C."));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field("editor")).count(), 3);
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field("editor")).first(), QLatin1String("Fielding, JE"));

  Tellico::Data::BibtexCollection* bColl = dynamic_cast<Tellico::Data::BibtexCollection*>(coll.data());
  QVERIFY(bColl);
  QCOMPARE(bColl->fieldByBibtexName("entry-type")->name(), QLatin1String("entry-type"));
}
