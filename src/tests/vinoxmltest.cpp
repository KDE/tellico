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

#include "vinoxmltest.h"

#include "../translators/vinoxmlimporter.h"
#include "../collections/winecollection.h"
#include "../collectionfactory.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../images/imageinfo.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( VinoXMLTest )

void VinoXMLTest::initTestCase() {
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/vinoxml2tellico.xsl"));
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::WineCollection> registerWine(Tellico::Data::Collection::Wine, "wine");
  Tellico::ImageFactory::init();
}

void VinoXMLTest::testImport() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test-vinoxml.xml"));
  Tellico::Import::VinoXMLImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(importer.canImport(Tellico::Data::Collection::Wine));

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Wine);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entries().first();
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("2002 Goldwater Estate Merlot"));
  QCOMPARE(entry->field("producer"), QStringLiteral("Goldwater Estate"));
  QCOMPARE(entry->field("vintage"), QStringLiteral("2002"));
  QCOMPARE(entry->field("varietal"), QStringLiteral("Merlot"));
  QCOMPARE(entry->field("appellation"), QStringLiteral("Waiheke Island"));
  QCOMPARE(entry->field("type"), QStringLiteral("Red Wine"));
  QCOMPARE(entry->field("pur_date"), QStringLiteral("2002-09-25"));
  QCOMPARE(entry->field("pur_price"), QStringLiteral("25.00"));
  QCOMPARE(entry->field("country"), QStringLiteral("New Zealand"));
  QCOMPARE(entry->field("quantity"), QStringLiteral("5"));
  QCOMPARE(entry->field("label"), QStringLiteral("GoldWaterEsslin2002.jpg"));
  QVERIFY(!entry->field("description").isEmpty());

  QVERIFY(Tellico::ImageFactory::validImage(entry->field("label")));
  Tellico::Data::ImageInfo info = Tellico::ImageFactory::imageInfo(entry->field(QStringLiteral("label")));
  QCOMPARE(info.width(false), 256);
  QCOMPARE(info.height(false), 920);
}
