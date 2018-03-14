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

#include "modstest.h"

#include "../translators/xsltimporter.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../fieldformat.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_APPLESS_MAIN( ModsTest )

void ModsTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/mods2tellico.xsl"));
}

void ModsTest::testBook() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/example_mods.xml"));
  Tellico::Import::XSLTImporter importer(url);
  importer.setXSLTURL(QUrl::fromLocalFile(QFINDTESTDATA("../../xslt/mods2tellico.xsl")));

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 1);
  QCOMPARE(coll->title(), QStringLiteral("MODS Import"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Sound and fury"));
  QCOMPARE(entry->field("author"), QStringLiteral("Alterman, Eric"));
  QCOMPARE(entry->field("genre"), QStringLiteral("bibliography"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("1999"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("0-8014-8639-4"));
  QCOMPARE(entry->field("lccn"), QStringLiteral("99042030"));
}
