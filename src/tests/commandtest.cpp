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

#include <QTest>
#include <QStandardPaths>
#include <QFile>

QTEST_GUILESS_MAIN( CommandTest )

void CommandTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");

  QVERIFY(m_tempDir.isValid());
  m_tempDir.setAutoRemove(true);
  auto tempDirName = m_tempDir.path();
  m_fileName = tempDirName + QStringLiteral("/with-image.tc");
  // copy a collection file that includes an image into the temporary directory
  QFile::copy(QFINDTESTDATA("data/with-image.tc"), m_fileName);
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

    // now undo it and check that everything returns to what it should be
    cmd.undo();
    QCOMPARE(Tellico::Data::Document::self()->URL(), docUrl);
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(!oldColl->hasField(test));

    cmd.redo();
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 2);
    QVERIFY(oldColl->hasField(test));

    cmd.undo();
    QCOMPARE(Tellico::Data::Document::self()->URL(), docUrl);
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(!oldColl->hasField(test));
  }
}

void CommandTest::testCollectionMerge() {
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

  {
    auto oldColl = doc->collection();
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

    // now undo it and check that everything returns to what it should be
    cmd.undo();
    QCOMPARE(Tellico::Data::Document::self()->URL(), docUrl);
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(!oldColl->hasField(test));

    cmd.redo();
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 2);
    QVERIFY(oldColl->hasField(test));

    cmd.undo();
    QCOMPARE(Tellico::Data::Document::self()->URL(), docUrl);
    QCOMPARE(doc->collection(), oldColl);
    QCOMPARE(oldColl->entryCount(), 1);
    QVERIFY(!oldColl->hasField(test));
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
