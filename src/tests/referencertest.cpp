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

#include "qtest_kde.h"
#include "referencertest.h"
#include "referencertest.moc"

#include "../translators/referencerimporter.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN_CORE( ReferencerTest )

void ReferencerTest::initTestCase() {
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
}

void ReferencerTest::testImport() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test.reflib");
  Tellico::Import::ReferencerImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 2);
  // should be translated somehow
  QCOMPARE(coll->title(), QLatin1String("Referencer Import"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("entry-type"), QLatin1String("article"));
  QCOMPARE(entry->field("year"), QLatin1String("2002"));
  QCOMPARE(entry->field("pages"), QLatin1String("1057-1119"));
  QCOMPARE(entry->fields("author", false).count(), 3);
  QCOMPARE(entry->fields("author", false).first(), QLatin1String("Koglin, M."));
  QCOMPARE(entry->field("entry-type"), QLatin1String("article"));
  QCOMPARE(entry->field("bibtex-key"), QLatin1String("Koglin2002"));
  QCOMPARE(entry->fields("keyword", false).count(), 2);
  QCOMPARE(entry->fields("keyword", false).first(), QLatin1String("tag1"));
}
