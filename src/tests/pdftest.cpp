/***************************************************************************
    Copyright (C) 2015 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>
#include "pdftest.h"

#include "../translators/pdfimporter.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../fieldformat.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( PdfTest )

void PdfTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBook(Tellico::Data::Collection::Bibtex, "bibliography");
  // since we use the XMP importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/xmp2tellico.xsl"));
}

void PdfTest::testScienceDirect() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-sciencedirect.pdf"));
  Tellico::Import::PDFImporter importer(url);

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
#ifdef HAVE_EXEMPI
  QCOMPARE(entry->field("title"), QLatin1String("Parametric analysis of air-water heat recovery concept applied to HVAC systems"));
  QCOMPARE(entry->field("author"), QLatin1String("Mohamad Ramadan; Mostafa Gad El Rab; Mahmoud Khaled"));
  QCOMPARE(entry->field("journal"), QLatin1String("Case Studies in Thermal Engineering"));
  QCOMPARE(entry->field("entry-type"), QLatin1String("article"));
  QCOMPARE(entry->field("publisher"), QLatin1String("Elsevier"));
  QCOMPARE(entry->field("year"), QLatin1String("2015"));
  QCOMPARE(entry->field("month"), QLatin1String("09"));
  QCOMPARE(entry->field("keyword"), QLatin1String("Heat recovery; HVAC; Water heating; Thermal modeling; Parametric analysis"));
  QCOMPARE(entry->field("volume"), QLatin1String("6"));
  QCOMPARE(entry->field("pages"), QLatin1String("61-68"));
  QCOMPARE(entry->field("doi"), QLatin1String("10.1016/j.csite.2015.06.001"));
//  QVERIFY(!entry->field("cover").isEmpty());
#endif
}
