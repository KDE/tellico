/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "commandtest.h"
#include "../commands/collectioncommand.h"
#include "../document.h"
#include "../translators/tellicoimporter.h"
#include "../collections/bookcollection.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"

#include <KLocalizedString>

#include <QTest>
#include <QStandardPaths>
#include <QFile>
#include <QLoggingCategory>

QTEST_GUILESS_MAIN( CommandTest )

void CommandTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");

  QVERIFY(m_tempDir.isValid());
  m_tempDir.setAutoRemove(true);
  auto tempDirName = m_tempDir.path();
  m_fileName = tempDirName + QStringLiteral("/with-image.tc");
  // copy a collection file that includes an image into the temporary directory
  QFile::copy(QFINDTESTDATA("data/with-image.tc"), m_fileName);

  QLoggingCategory::setFilterRules(QStringLiteral("tellico.debug = true\ntellico.info = true"));
}

void CommandTest::testCollectionReplace() {
  Tellico::Data::Document* doc = Tellico::Data::Document::self();
  QVERIFY(doc->openDocument(QUrl::fromLocalFile(m_fileName)));
  auto docUrl = doc->URL();
  QCOMPARE(QUrl::fromLocalFile(m_fileName), docUrl);

  Tellico::Data::CollPtr newColl(new Tellico::Data::BookCollection(true));

  {
    auto oldColl = doc->collection();
    Tellico::Command::CollectionCommand cmd(Tellico::Command::CollectionCommand::Replace,
                                            doc->collection(),
                                            newColl);
    cmd.redo();
    // doc url was erased
    QVERIFY(doc->URL() != docUrl);
    QCOMPARE(doc->collection(), newColl);

    // now undo it and check that everything returns to what it should be
    cmd.undo();
    QCOMPARE(Tellico::Data::Document::self()->URL(), docUrl);
    QCOMPARE(doc->collection(), oldColl);
  }
  //  the d'tor should clear the new collection (since the replace was undone)
  QVERIFY(newColl->fields().isEmpty());
}

void CommandTest::testCollectionAppend() {
  Tellico::Data::Document* doc = Tellico::Data::Document::self();
  QVERIFY(doc->openDocument(QUrl::fromLocalFile(m_fileName)));
  auto docUrl = doc->URL();
  QCOMPARE(QUrl::fromLocalFile(m_fileName), docUrl);

  auto test = QStringLiteral("test");

  Tellico::Data::CollPtr newColl(new Tellico::Data::BookCollection(true));
  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(test, test));
  newColl->addField(field1);
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(newColl));
  newColl->addEntries(entry1);
  entry1->setField(test, QStringLiteral("test value"));

  {
    auto oldColl = doc->collection();
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(!oldColl->hasField(test));

    Tellico::Command::CollectionCommand cmd(Tellico::Command::CollectionCommand::Append,
                                            doc->collection(),
                                            newColl);
    cmd.redo();
    // collection pointer did not change
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 2);
    QVERIFY(oldColl->hasField(test));

    // save the appended entry and verify that redo restores the same entry
    auto appendedEntry = oldColl->entries().constLast();
    QVERIFY(appendedEntry);
    const int appendedEntryId = appendedEntry->id();
    QVERIFY(appendedEntryId >= 0);
    QCOMPARE(appendedEntry->field(test), QStringLiteral("test value"));

    // now undo it and check that everything returns to what it should be
    cmd.undo();
    QCOMPARE(Tellico::Data::Document::self()->URL(), docUrl);
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(!oldColl->hasField(test));
    QVERIFY(!oldColl->entries().contains(appendedEntry));

    cmd.redo();
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 2);
    QVERIFY(oldColl->hasField(test));
    // redo should re-add the original entry rather than create a new copy
    QVERIFY(oldColl->entries().contains(appendedEntry));
    QCOMPARE(appendedEntry->id(), appendedEntryId);
    QCOMPARE(appendedEntry->collection(), oldColl);
    QCOMPARE(appendedEntry->field(test), QStringLiteral("test value"));

    cmd.undo();
    QCOMPARE(Tellico::Data::Document::self()->URL(), docUrl);
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(!oldColl->hasField(test));
    QVERIFY(!oldColl->entries().contains(appendedEntry));
  }
}

void CommandTest::testCollectionAppend2() {
  QUrl url1 = QUrl::fromLocalFile(QFINDTESTDATA("data/test-merge-filtbor-01.tc"));
  QUrl url2 = QUrl::fromLocalFile(QFINDTESTDATA("data/test-merge-filtbor-02.tc"));;

  QVERIFY(QFile::exists(url1.toLocalFile()));
  QVERIFY(QFile::exists(url2.toLocalFile()));

  auto doc = Tellico::Data::Document::self();
  QVERIFY(doc->openDocument(url1));
  auto targetColl = doc->collection();
  QCOMPARE(targetColl->borrowers().count(), 3);
  QCOMPARE(targetColl->filters().count(), 3);

  Tellico::Import::TellicoImporter importer2(url2);
  auto appendColl = importer2.collection();
  QCOMPARE(appendColl->borrowers().count(), 4);
  QCOMPARE(appendColl->filters().count(), 4);

  Tellico::CollectionMergeOptions opt;
  opt.importLoans = true;
  opt.importFilters = true;

  Tellico::Command::CollectionCommand cmd(Tellico::Command::CollectionCommand::Append,
                                          targetColl,
                                          appendColl,
                                          opt);
  cmd.redo();

  // two filters have the same name but different rules so a new one is created
  // two filters are identical, so total ends up being 6
  QCOMPARE(targetColl->filters().count(), 6);
  auto findFilterByName = [](const Tellico::FilterList& filters_, const QString& name_) -> Tellico::FilterPtr {
    const auto it = std::find_if(filters_.constBegin(), filters_.constEnd(),
                                 [&name_](const auto& filter_) {
      return filter_ && filter_->name() == name_;
    });
    return it == filters_.constEnd() ? Tellico::FilterPtr() : *it;
  };

  auto filter = findFilterByName(targetColl->filters(), QStringLiteral("BF_109_110"));
  QVERIFY(filter);
  QCOMPARE(filter->op(), Tellico::Filter::MatchAny);
  QCOMPARE(filter->count(), 2);

  // add (1) to the name
  filter = findFilterByName(targetColl->filters(), QStringLiteral("BF_109_110 (1)"));
  QVERIFY(filter);
  QCOMPARE(filter->op(), Tellico::Filter::MatchAll);
  QCOMPARE(filter->count(), 2);

  filter = findFilterByName(targetColl->filters(), QStringLiteral("FIAT_aircraft"));
  QVERIFY(filter);
  QCOMPARE(filter->count(), 1);

  Tellico::Data::BorrowerPtr bor;
  // one borrower has the same name, so appending ends up with 6
  QCOMPARE(targetColl->borrowers().count(), 6);
  for(auto mergeBorrower : targetColl->borrowers()) {
    const auto testName = mergeBorrower->name();
    int borrowerLoanCount = mergeBorrower->loans().count();
    if(testName == QStringLiteral("Annibale Barca")) {
      bor = mergeBorrower;
      QVERIFY2(borrowerLoanCount == 6,
               qPrintable(QStringLiteral("Mismatch loans for %1: %2 instead of 6").arg(testName).arg(borrowerLoanCount)));
    } else if(testName == QStringLiteral("Caio Iulio Cesare")) {
      QVERIFY2(borrowerLoanCount == 2,
               qPrintable(QStringLiteral("Mismatch loans for %1: %2 instead of 2").arg(testName).arg(borrowerLoanCount)));
    } else if(testName == QStringLiteral("Cleopatra Tèa Filopàtore")) {
      QVERIFY2(borrowerLoanCount == 1,
               qPrintable(QStringLiteral("Mismatch loans for %1: %2 instead of 1").arg(testName).arg(borrowerLoanCount)));
    } else if(testName == QStringLiteral("Germanico Giulio Cesare")) {
      QVERIFY2(borrowerLoanCount == 5,
               qPrintable(QStringLiteral("Mismatch loans for %1: %2 instead of 5").arg(testName).arg(borrowerLoanCount)));
    } else if(testName == QStringLiteral("Marco Antonio")) {
      QVERIFY2(borrowerLoanCount == 1,
               qPrintable(QStringLiteral("Mismatch loans for %1: %2 instead of 1").arg(testName).arg(borrowerLoanCount)));
    } else if(testName == QStringLiteral("Publio Cornelio Scipione")) {
      QVERIFY2(borrowerLoanCount == 12,
               qPrintable(QStringLiteral("Mismatch loans for %1: %2 instead of 12").arg(testName).arg(borrowerLoanCount)));
    }
  }

  cmd.undo();
  filter = findFilterByName(targetColl->filters(), QStringLiteral("BF_109_110 (1)"));
  QVERIFY(!filter); // no longer there
  QCOMPARE(targetColl->borrowers().count(), 3);
  QVERIFY(bor);
  QVERIFY(targetColl->borrowers().contains(bor));
  QCOMPARE(bor->loans().count(), 4);

  cmd.redo();
  filter = findFilterByName(targetColl->filters(), QStringLiteral("BF_109_110 (1)"));
  QVERIFY(filter);
  QCOMPARE(targetColl->borrowers().count(), 6);
  QVERIFY(bor);
  QVERIFY(targetColl->borrowers().contains(bor));
  QCOMPARE(bor->loans().count(), 6);
}

void CommandTest::testCollectionMerge() {
  Tellico::Data::Document* doc = Tellico::Data::Document::self();
  QVERIFY(doc->openDocument(QUrl::fromLocalFile(m_fileName)));
  auto docUrl = doc->URL();
  QCOMPARE(QUrl::fromLocalFile(m_fileName), docUrl);

  const auto test = QStringLiteral("test");

  Tellico::Data::CollPtr newColl(new Tellico::Data::BookCollection(true));
  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(test, test));
  newColl->addField(field1);
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(newColl));
  newColl->addEntries(entry1);

  auto oldColl = doc->collection();
  // modify an existing field
  auto existingField = oldColl->fieldByName(QStringLiteral("isbn"));
  QVERIFY(existingField);

  // add the same book entry with some different values
  auto existingEntry = oldColl->entries().first();
  const auto author = existingEntry->field(QStringLiteral("author"));
  const auto mdate = existingEntry->field(QStringLiteral("mdate"));
  const auto pages = existingEntry->field(QStringLiteral("pages"));
  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(newColl));
  entry2->setField(QStringLiteral("title"), existingEntry->field(QStringLiteral("title")));
  // so the merge will see perfect match, use exact isbn
  entry2->setField(QStringLiteral("isbn"), existingEntry->field(QStringLiteral("isbn")));
  entry2->setField(QStringLiteral("author"), author + QStringLiteral("; Author2"));
  // add translator name as new value
  entry2->setField(QStringLiteral("translator"), QStringLiteral("Mr. Translator"));
  // have a different pages value and check it doesn't get changed
  entry2->setField(QStringLiteral("pages"), QStringLiteral("4000"));
  newColl->addEntries(entry2);

  {
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(!oldColl->hasField(test));

    Tellico::Command::CollectionCommand cmd(Tellico::Command::CollectionCommand::Merge,
                                            doc->collection(),
                                            newColl);
    cmd.redo();
    // collection pointer did not change
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 2);
    QVERIFY(oldColl->hasField(test));
    QVERIFY(existingEntry);
    QEXPECT_FAIL("", "Field values are not currently merged", Continue);
    QCOMPARE(existingEntry->field(QStringLiteral("author")),
             entry2->field(QStringLiteral("author")));
    QCOMPARE(existingEntry->field(QStringLiteral("translator")),
             QStringLiteral("Mr. Translator"));
    QCOMPARE(existingEntry->field(QStringLiteral("pages")),
             pages);
    const auto newMDate = existingEntry->field(QStringLiteral("mdate"));
    QVERIFY(mdate != newMDate);

    // now undo it and check that everything returns to what it should be
    cmd.undo();
    QCOMPARE(Tellico::Data::Document::self()->URL(), docUrl);
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(!oldColl->hasField(test));
    QCOMPARE(existingEntry->field(QStringLiteral("author")), author);
    QVERIFY(existingEntry->field(QStringLiteral("translator")).isEmpty());
    // restored mdate
    QCOMPARE(existingEntry->field(QStringLiteral("mdate")), mdate);

    cmd.redo();
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 2);
    QVERIFY(oldColl->hasField(test));
    QCOMPARE(existingEntry->field(QStringLiteral("translator")),
             QStringLiteral("Mr. Translator"));
    QCOMPARE(existingEntry->field(QStringLiteral("pages")),
             pages);

    cmd.undo();
    QCOMPARE(Tellico::Data::Document::self()->URL(), docUrl);
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(!oldColl->hasField(test));
    QVERIFY(existingEntry->field(QStringLiteral("translator")).isEmpty());
    // restored mdate
    QCOMPARE(existingEntry->field(QStringLiteral("mdate")), mdate);
  }
}

void CommandTest::testBibtexCollectionAppend() {
  auto bibtexColl1 = new Tellico::Data::BibtexCollection(true);
  // by default includes 12 months
  QCOMPARE(bibtexColl1->macroList().count(), 12);
  QVERIFY(bibtexColl1->preamble().isEmpty());
  Tellico::Data::CollPtr oldColl(bibtexColl1);

  Tellico::Data::Document* doc = Tellico::Data::Document::self();
  doc->replaceCollection(oldColl);

  auto test = QStringLiteral("test");

  auto bibtexColl2 = new Tellico::Data::BibtexCollection(true);
  bibtexColl2->addMacro(test, test);
  bibtexColl2->setPreamble(test);
  QCOMPARE(bibtexColl2->macroList().count(), 13);
  QCOMPARE(bibtexColl2->preamble(), test);
  Tellico::Data::CollPtr newColl(bibtexColl2);
  Tellico::Data::FieldPtr field1(new Tellico::Data::Field(test, test));
  newColl->addField(field1);
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(newColl));
  newColl->addEntries(entry1);

  {
    auto oldColl = doc->collection();
    QCOMPARE(oldColl->entryCount(), 0);
    QVERIFY(!oldColl->hasField(test));
    QVERIFY(bibtexColl1->preamble().isEmpty());

    Tellico::Command::CollectionCommand cmd(Tellico::Command::CollectionCommand::Append,
                                            doc->collection(),
                                            newColl);
    cmd.redo();
    // collection pointer did not change
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(oldColl->hasField(test));
    QCOMPARE(bibtexColl1->macroList().count(), 13);
    QCOMPARE(bibtexColl1->preamble(), test);

    // now undo it and check that everything returns to what it should be
    cmd.undo();
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 0);
    QVERIFY(!oldColl->hasField(test));
    QCOMPARE(bibtexColl1->macroList().count(), 12);
    QVERIFY(bibtexColl1->preamble().isEmpty());
  }
}
