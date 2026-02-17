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

#include <config.h>
#include "bibtextest.h"

#include "../collections/bibtexcollection.h"
#include "../translators/bibteximporter.h"
#include "../translators/bibtexexporter.h"
#include "../utils/bibtexhandler.h"
#include "../utils/datafileregistry.h"

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QTest>

QTEST_GUILESS_MAIN( BibtexTest )

#define QL1(x) QStringLiteral(x)

void BibtexTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
  // since we use the bibtex mapping file
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../translators/bibtex-translation.xml"));
}

void BibtexTest::testImport() {
#ifdef ENABLE_BTPARSE
  KSharedConfigPtr config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig);
  KConfigGroup cg = config->group(QStringLiteral("ExportOptions - Bibtex"));
  cg.writeEntry("URL Package", true);

  QList<QUrl> urls;
  urls << QUrl::fromLocalFile(QFINDTESTDATA("data/test.bib"));

  Tellico::Import::BibtexImporter importer(urls);
  // shut the importer up about current collection
  Tellico::Data::CollPtr tmpColl(new Tellico::Data::BibtexCollection(true));
  importer.setCurrentCollection(tmpColl);

  Tellico::Data::CollPtr coll = importer.collection();
  Tellico::Data::BibtexCollection* bColl = static_cast<Tellico::Data::BibtexCollection*>(coll.data());

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 36);
  QVERIFY(coll->hasField(QStringLiteral("issn")));

  QCOMPARE(bColl->preamble(), QL1("preamble"));

  Tellico::Data::EntryPtr entry = coll->entryById(2);
  QCOMPARE(entry->field("entry-type"), QL1("article"));
  QCOMPARE(entry->field("bibtex-key"), QL1("article-full"));
  QCOMPARE(entry->field("author"), QString::fromUtf8("L[eslie] A. Aamport"));
  QCOMPARE(entry->field("month"), QL1("jul"));
  QCOMPARE(entry->field("url"), QL1("http://example.com/~user/"));
  QCOMPARE(entry->field("keyword"), QL1("keyword1; keyword2; keyword3"));
  QCOMPARE(entry->field("issn"), QL1("1334-2345"));
  QCOMPARE(bColl->macroList().value("ACM"), QL1("The OX Association for Computing Machinery"));

  Tellico::BibtexHandler::s_quoteStyle = Tellico::BibtexHandler::QUOTES;

  Tellico::Export::BibtexExporter exporter(coll);
  exporter.setEntries(coll->entries());
  exporter.readOptions(config);
  Tellico::Import::BibtexImporter importer2(exporter.text());
  importer2.setCurrentCollection(tmpColl);
  Tellico::Data::CollPtr coll2 = importer2.collection();
  Tellico::Data::BibtexCollection* bColl2 = static_cast<Tellico::Data::BibtexCollection*>(coll2.data());

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = bColl2->entryByBibtexKey(e1->field(QStringLiteral("bibtex-key")));
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      // entry ids will be different
      if(f->name() != QStringLiteral("id")) {
        QCOMPARE(f->name() + e1->field(f), f->name() + e2->field(f));
      }
    }
  }
#endif
}

void BibtexTest::testPages() {
  // small test to check the pages value ends up with 2 hyphens
  Tellico::Data::CollPtr coll(new Tellico::Data::BibtexCollection(true));
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  coll->addEntries(entry1);
  entry1->setField(QStringLiteral("title"), QStringLiteral("Title 1"));
  entry1->setField(QStringLiteral("entry-type"), QStringLiteral("article"));
  entry1->setField(QStringLiteral("pages"), QStringLiteral("2-8"));

  Tellico::Export::BibtexExporter exporter(coll);
  exporter.setEntries(coll->entries());
  QString text = exporter.text();
  QVERIFY(text.contains(QStringLiteral("2--8")));
}

void BibtexTest::testDuplicateKeys() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BibtexCollection(true));
  Tellico::Data::BibtexCollection* bColl = static_cast<Tellico::Data::BibtexCollection*>(coll.data());

  Tellico::Data::EntryList dupes = bColl->duplicateBibtexKeys();
  QVERIFY(dupes.isEmpty());

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QStringLiteral("title"), QStringLiteral("Title 1"));
  entry1->setField(QStringLiteral("bibtex-key"), QStringLiteral("title1"));

  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  entry2->setField(QStringLiteral("title"), QStringLiteral("Title 2"));
  entry2->setField(QStringLiteral("bibtex-key"), QStringLiteral("title1"));

  Tellico::Data::EntryPtr entry3(new Tellico::Data::Entry(coll));
  entry3->setField(QStringLiteral("title"), QStringLiteral("Title 3"));
  entry3->setField(QStringLiteral("bibtex-key"), QStringLiteral("title3"));

  coll->addEntries(Tellico::Data::EntryList() << entry1 << entry2 << entry3);

  QCOMPARE(coll->entries().count(), 3);

  dupes = bColl->duplicateBibtexKeys();
  QCOMPARE(dupes.count(), 2);

  entry2->setField(QStringLiteral("bibtex-key"), QStringLiteral("title2"));
  dupes = bColl->duplicateBibtexKeys();
  QCOMPARE(dupes.count(), 0);
}

void BibtexTest::testMapping() {
  // test a few strings from the bibtex mapping file
  QCOMPARE(Tellico::BibtexHandler::exportText(QString::fromUtf8("™"), QStringList()), QStringLiteral("{{\\texttrademark}}"));
  QCOMPARE(Tellico::BibtexHandler::exportText(QString::fromUtf8("ß"), QStringList()), QStringLiteral("{{\\ss}}"));
  QCOMPARE(Tellico::BibtexHandler::exportText(QString::fromUtf8("…"), QStringList()), QStringLiteral("{{\\ldots}}"));
  QCOMPARE(Tellico::BibtexHandler::exportText(QString::fromUtf8("°"), QStringList()), QStringLiteral("{$^{\\circ}$}"));
}

void BibtexTest::testMaybe() {
#ifdef ENABLE_BTPARSE
  QUrl u(QUrl::fromLocalFile(QFINDTESTDATA("data/test.bib")));
  QVERIFY(Tellico::Import::BibtexImporter::maybeBibtex(u));
#endif
}

void BibtexTest::testModify() {
  Tellico::Data::BibtexCollection coll(true); // add default fields
  auto field = coll.fieldByBibtexName(QStringLiteral("key"));
  QVERIFY(field);
  QCOMPARE(field->name(), QLatin1String("bibtex-key"));

  field->setProperty(QStringLiteral("bibtex"), QStringLiteral("new-key"));
  QVERIFY(coll.modifyField(field));

  field = coll.fieldByBibtexName(QStringLiteral("key"));
  QVERIFY(!field); // no longer exists in collection by this bibtex name
  field = coll.fieldByBibtexName(QStringLiteral("new-key"));
  QVERIFY(field);
  QCOMPARE(field->name(), QLatin1String("bibtex-key"));
}
