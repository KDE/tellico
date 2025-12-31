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

#include "ristest.h"

#include "../translators/risimporter.h"
#include "../collections/bibtexcollection.h"
#include "../fieldformat.h"

#include <KLocalizedString>

#include <QTest>

QTEST_APPLESS_MAIN( RisTest )

void RisTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
}

void RisTest::testImport() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test.ris"));
  QList<QUrl> urls;
  urls << url;
  Tellico::Import::RISImporter importer(urls);
  // shut the importer up about current collection
  Tellico::Data::CollPtr tmpColl(new Tellico::Data::BibtexCollection(true));
  importer.setCurrentCollection(tmpColl);

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 2);
  QCOMPARE(coll->title(), QStringLiteral("Bibliography"));
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("year"), QStringLiteral("2002"));
  QCOMPARE(entry->field("pages"), QStringLiteral("159-223"));
  QCOMPARE(entry->field("volume"), QStringLiteral("230"));
  QCOMPARE(entry->field("journal"), QStringLiteral("Mol Cell Biochem"));
  QVERIFY(entry->field("address").contains(QStringLiteral("Germany")));
  QCOMPARE(entry->field("pdf"), QStringLiteral("test.pdf"));

  entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("entry-type"), QStringLiteral("article"));
  QCOMPARE(entry->field("year"), QStringLiteral("2002"));
  QCOMPARE(entry->field("pages"), QStringLiteral("1057-1119"));
  const auto authors = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("author")));
  QCOMPARE(authors.count(), 3);
  QVERIFY(authors.contains(QStringLiteral("Koglin,M.")));
  const auto keywords = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("keyword")));
  QCOMPARE(keywords.count(), 37);
  QVERIFY(keywords.contains(QStringLiteral("Human")));

  auto bColl = dynamic_cast<Tellico::Data::BibtexCollection*>(coll.data());
  QVERIFY(bColl);
  QCOMPARE(bColl->fieldByBibtexName("entry-type")->name(), QStringLiteral("entry-type"));
  QVERIFY(Tellico::Import::RISImporter::maybeRIS(url));
}
