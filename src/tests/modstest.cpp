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
#include "modstest.moc"
#include "qtest_kde.h"

#include "../translators/xsltimporter.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../fieldformat.h"

#include <KStandardDirs>

QTEST_KDEMAIN( ModsTest, GUI )

void ModsTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  // since we use the MODS importer
//  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
}

void ModsTest::testBook() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/example_mods.xml");
  Tellico::Import::XSLTImporter importer(url);
  importer.setXSLTURL(QString::fromLatin1(KDESRCDIR) + "/../../xslt/mods2tellico.xsl");

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 1);
  QCOMPARE(coll->title(), QLatin1String("MODS Import"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QLatin1String("Sound and fury"));
  QCOMPARE(entry->field("author"), QLatin1String("Alterman, Eric"));
  QCOMPARE(entry->field("genre"), QLatin1String("bibliography"));
  QCOMPARE(entry->field("pub_year"), QLatin1String("1999"));
  QCOMPARE(entry->field("isbn"), QLatin1String("0-8014-8639-4"));
  QCOMPARE(entry->field("lccn"), QLatin1String("99042030"));
}
