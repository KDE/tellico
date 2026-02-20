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
#include "../translators/ebookimporter.h"
#include "../collections/bibtexcollection.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KLocalizedString>
#ifdef HAVE_KFILEMETADATA
#include <KFileMetaData/ExtractorCollection>
#endif

#include <QTest>
#include <QStandardPaths>

// needs a GUI for QPixmaps
QTEST_MAIN( PdfTest )

#ifdef HAVE_KFILEMETADATA
static bool kfilemetadataCanReadPdf() {
  const KFileMetaData::ExtractorCollection collection;
  const QList<KFileMetaData::Extractor *> extractors = collection.fetchExtractors(QStringLiteral("application/pdf"));
  return !extractors.isEmpty();
}
#endif

void PdfTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibliography");
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  // since we use the XMP importer
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/xmp2tellico.xsl"));
  Tellico::ImageFactory::init();
}

void PdfTest::testScienceDirect() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-sciencedirect.pdf"));
  Tellico::Import::PDFImporter importer(url);
  QVERIFY(importer.canImport(Tellico::Data::Collection::Book));
  QVERIFY(importer.canImport(Tellico::Data::Collection::Bibtex));

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
#ifdef HAVE_EXEMPI
  QCOMPARE(entry->field("title"), QStringLiteral("Parametric analysis of air-water heat recovery concept applied to HVAC systems"));
  QCOMPARE(entry->field("author"), QStringLiteral("Mohamad Ramadan; Mostafa Gad El Rab; Mahmoud Khaled"));
  QCOMPARE(entry->field("journal"), QStringLiteral("Case Studies in Thermal Engineering"));
  QCOMPARE(entry->field("entry-type"), QStringLiteral("article"));
  QCOMPARE(entry->field("publisher"), QStringLiteral("Elsevier"));
  QCOMPARE(entry->field("year"), QStringLiteral("2015"));
  QCOMPARE(entry->field("month"), QStringLiteral("09"));
  QCOMPARE(entry->field("keyword"), QStringLiteral("Heat recovery; HVAC; Water heating; Thermal modeling; Parametric analysis"));
  QCOMPARE(entry->field("volume"), QStringLiteral("6"));
  QCOMPARE(entry->field("pages"), QStringLiteral("61-68"));
  QCOMPARE(entry->field("doi"), QStringLiteral("10.1016/j.csite.2015.06.001"));
//  QVERIFY(!entry->field("cover").isEmpty());
#endif
}

void PdfTest::testMultiple() {
  const QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-sciencedirect.pdf"));
  for (int i = 0; i < 5; ++i) {
    Tellico::Import::PDFImporter importer(url);
    Tellico::Data::CollPtr coll = importer.collection();
    QVERIFY(coll);
  }
}

void PdfTest::testMetadata() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-metadata.pdf"));
  Tellico::Import::PDFImporter importer(url);

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
#ifdef HAVE_POPPLER
  QCOMPARE(entry->field("title"), QStringLiteral("The Big Brown Bear"));
  QCOMPARE(entry->field("author"), QStringLiteral("Happy Man"));
  QCOMPARE(entry->field("entry-type"), QStringLiteral("article"));
  QCOMPARE(entry->field("keyword"), QStringLiteral("PDF Metadata"));
#endif
}

void PdfTest::testBookCollection() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-metadata.pdf"));
  Tellico::Import::PDFImporter importer(url);

  // PDF importer defaults to bibtex unless current collection is a book collection
  Tellico::Data::CollPtr tmpColl(new Tellico::Data::BookCollection(true));
  importer.setCurrentCollection(tmpColl);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
#ifdef HAVE_POPPLER
  QCOMPARE(entry->field("title"), QStringLiteral("The Big Brown Bear"));
  QCOMPARE(entry->field("author"), QStringLiteral("Happy Man"));
  QCOMPARE(entry->field("keyword"), QStringLiteral("PDF Metadata"));
#endif
}

void PdfTest::testBookCollectionMetadata() {
#ifdef HAVE_KFILEMETADATA
  if (!kfilemetadataCanReadPdf()) {
    QSKIP("KFileMetaData does not have plugins to extract PDF metadata.", SkipAll);
  }

  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-metadata.pdf"));
  Tellico::Import::EBookImporter importer(QList<QUrl>() << url);

  // PDF importer defaults to bibtex unless current collection is a book collection
  Tellico::Data::CollPtr tmpColl(new Tellico::Data::BookCollection(true));
  importer.setCurrentCollection(tmpColl);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Big Brown Bear"));
  QCOMPARE(entry->field("author"), QStringLiteral("Happy Man"));
  QCOMPARE(entry->field("keyword"), QStringLiteral("PDF Metadata"));
  QCOMPARE(entry->field("comments"), url.toLocalFile());
#endif
}
