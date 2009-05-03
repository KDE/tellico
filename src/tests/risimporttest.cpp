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
#include "risimporttest.h"
#include "risimporttest.moc"

#include "../translators/risimporter.h"
#include "../collections/bibtexcollection.h"

QTEST_KDEMAIN_CORE( RisImportTest )

void RisImportTest::testImport() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test.ris");
  Tellico::Import::RISImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 3);

  Tellico::Data::EntryPtr entry = coll->entryById(0);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("entry-type"), QLatin1String("article"));
  QCOMPARE(entry->field("year"), QLatin1String("2002"));
  QCOMPARE(entry->field("pages"), QLatin1String("2276-2357"));
  QCOMPARE(entry->fields("author", false).count(), 5);
  QCOMPARE(entry->fields("author", false).first(), QLatin1String("Mizusawa,H."));

  Tellico::Data::BibtexCollection* bColl = dynamic_cast<Tellico::Data::BibtexCollection*>(coll.data());
  QVERIFY(bColl);
  QCOMPARE(bColl->fieldByBibtexName("entry-type")->name(), QLatin1String("entry-type"));
}
