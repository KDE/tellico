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

#include "bibtextest.h"
#include "bibtextest.moc"
#include "qtest_kde.h"

#include "../translators/bibteximporter.h"
#include "../collections/bibtexcollection.h"
#include "../translators/bibtexexporter.h"
#include "../translators/bibtexhandler.h"

#include <kstandarddirs.h>

QTEST_KDEMAIN_CORE( BibtexTest )

#define QL1(x) QString::fromLatin1(x)

void BibtexTest::initTestCase() {
  // since we use the bibtex importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../translators/");
}

void BibtexTest::testImport() {
  KUrl::List urls;
  urls << KUrl(QString::fromLatin1(KDESRCDIR) + "/data/test.bib");

  Tellico::Import::BibtexImporter importer(urls);
  // shut the importer up about current collection
  Tellico::Data::CollPtr tmpColl(new Tellico::Data::BibtexCollection(true));
  importer.setCurrentCollection(tmpColl);

  Tellico::Data::CollPtr coll = importer.collection();
  Tellico::Data::BibtexCollection* bColl = static_cast<Tellico::Data::BibtexCollection*>(coll.data());

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 36);

  QCOMPARE(bColl->preamble(), QL1("preamble"));

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QCOMPARE(entry->field("entry-type"), QL1("article"));
  QCOMPARE(entry->field("bibtex-key"), QL1("article-full"));
  QCOMPARE(entry->field("author"), QL1("L[eslie] A. Aamport"));
  QCOMPARE(entry->field("month"), QL1("jul"));
  QCOMPARE(entry->field("keyword"), QL1("keyword1; keyword2; keyword3"));
  QCOMPARE(bColl->macroList().value("ACM"), QL1("The OX Association for Computing Machinery"));

  Tellico::BibtexHandler::s_quoteStyle = Tellico::BibtexHandler::QUOTES;

  Tellico::Export::BibtexExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::BibtexImporter importer2(exporter.text());
  importer2.setCurrentCollection(tmpColl);
  Tellico::Data::CollPtr coll2 = importer2.collection();
  Tellico::Data::BibtexCollection* bColl2 = static_cast<Tellico::Data::BibtexCollection*>(coll2.data());

  QVERIFY(!coll2.isNull());
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = bColl2->entryByBibtexKey(e1->field(QLatin1String("bibtex-key")));
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // entry ids will be different
      if(f->name() != QLatin1String("id")) {
        QCOMPARE(f->name() + e1->field(f), f->name() + e2->field(f));
      }
    }
  }
}
