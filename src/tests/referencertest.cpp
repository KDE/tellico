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

#undef QT_NO_CAST_FROM_ASCII

#include "referencertest.h"

#include "../translators/referencerimporter.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../fieldformat.h"
#include "../utils/datafileregistry.h"

#include <KLocalizedString>

#include <QTest>

QTEST_GUILESS_MAIN( ReferencerTest )

void ReferencerTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/referencer2tellico.xsl"));
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
}

void ReferencerTest::testImport() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test.reflib"));
  Tellico::Import::ReferencerImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 2);
  // should be translated somehow
  QCOMPARE(coll->title(), QStringLiteral("Referencer Import"));

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("entry-type"), QStringLiteral("article"));
  QCOMPARE(entry->field("year"), QStringLiteral("2002"));
  QCOMPARE(entry->field("pages"), QStringLiteral("1057-1119"));
  const auto authors = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("author")));
  QCOMPARE(authors.count(), 3);
  QCOMPARE(authors.first(), QStringLiteral("Koglin, M."));
  QCOMPARE(entry->field("entry-type"), QStringLiteral("article"));
  QCOMPARE(entry->field("bibtex-key"), QStringLiteral("Koglin2002"));
  const auto keywords = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("keyword")));
  QCOMPARE(keywords.count(), 2);
  QCOMPARE(keywords.first(), QStringLiteral("tag1"));
}
