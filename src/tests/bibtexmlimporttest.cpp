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
#include "bibtexmlimporttest.h"
#include "bibtexmlimporttest.moc"

#include "../translators/bibtexmlimporter.h"
#include "../collections/bibtexcollection.h"

QTEST_KDEMAIN_CORE( BibtexmlImportTest )

#define QL1(x) QString::fromLatin1(x)

void BibtexmlImportTest::testImport() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test.bibtexml");

  Tellico::Import::BibtexmlImporter importer(url);
  // shut the importer up about current collection
  Tellico::Data::CollPtr tmpColl(new Tellico::Data::BibtexCollection(true));
  importer.setCurrentCollection(tmpColl);

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 2);

  Tellico::Data::EntryPtr entry = coll->entryById(0);
  QCOMPARE(entry->field("entry-type"), QL1("book"));
  QCOMPARE(entry->field("bibtex-key"), QL1("esbensen"));
  QCOMPARE(entry->field("author"), QL1("Kim Esbensen; Tonje Midtgaard"));
}
