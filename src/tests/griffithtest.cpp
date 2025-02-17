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

#include "griffithtest.h"

#include "../translators/griffithimporter.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KLocalizedString>

#include <QTest>
#include <QStandardPaths>

#define QSL(x) QStringLiteral(x)

QTEST_GUILESS_MAIN( GriffithTest )

void GriffithTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/griffith2tellico.xsl"));
  Tellico::ImageFactory::init();
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
}

void GriffithTest::testMovies() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/griffith.xml"));
  Tellico::Import::GriffithImporter importer(url);
  // can't import images for local test
  importer.setOptions(importer.options() & ~Tellico::Import::ImportShowImageErrors);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 5);
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field(QSL("title")), QSL("Serendipity"));
  QCOMPARE(entry->field(QSL("origtitle")), QSL("Serendipity"));
  QCOMPARE(entry->field(QSL("director")), QSL("Peter Chelsom"));
  QCOMPARE(entry->field(QSL("year")), QSL("2001"));
  QCOMPARE(entry->field(QSL("certification")), QSL("PG-13 (USA)"));
  QCOMPARE(entry->field(QSL("nationality")), QSL("USA"));
  QCOMPARE(entry->field(QSL("genre")), QSL("Comedy; Romance; Fantasy"));
  QCOMPARE(entry->field(QSL("rating")), QSL("6"));
  QCOMPARE(entry->field(QSL("running-time")), QSL("90"));
  QCOMPARE(entry->field(QSL("studio")), QSL("studio"));
  QCOMPARE(entry->field(QSL("seen")), QSL("true"));
  QCOMPARE(entry->field(QSL("medium")), QSL("DVD"));
  const auto cast = Tellico::FieldFormat::splitTable(entry->field(QSL("cast")));
  QCOMPARE(cast.first(), QSL("John Cusack::Jonathan Trager"));
  QVERIFY(!entry->field(QSL("plot")).isEmpty());
  // cover will be empty since local images don't exist
}
