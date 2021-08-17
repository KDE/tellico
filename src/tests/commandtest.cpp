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
#include "../collectionfactory.h"
#include "../images/imagefactory.h"

#include <QTest>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QFile>

QTEST_GUILESS_MAIN( CommandTest )

void CommandTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
}

void CommandTest::testCollectionReplace() {
  QString tempDirName;
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  tempDir.setAutoRemove(true);
  tempDirName = tempDir.path();
  QString fileName = tempDirName + QStringLiteral("/with-image.tc");

  // copy a collection file that includes an image into the temporary directory
  QVERIFY(QFile::copy(QFINDTESTDATA("data/with-image.tc"), fileName));

  Tellico::Data::Document* doc = Tellico::Data::Document::self();
  QVERIFY(doc->openDocument(QUrl::fromLocalFile(fileName)));
  auto docUrl = Tellico::Data::Document::self()->URL();
  QCOMPARE(QUrl::fromLocalFile(fileName), docUrl);

  auto newBookCollection = new Tellico::Data::BookCollection(true);
  {
    Tellico::Command::CollectionCommand cmd(Tellico::Command::CollectionCommand::Replace,
                                            Tellico::Data::Document::self()->collection(),
                                            Tellico::Data::CollPtr(newBookCollection));
    cmd.redo();
    // doc url was erased
    QVERIFY(Tellico::Data::Document::self()->URL() != docUrl);

    // now undo it and check that everythign returns to what it should be
    cmd.undo();
    QCOMPARE(Tellico::Data::Document::self()->URL(), docUrl);
  }
  //  the d'tor should clear the new collection (since the replace was undone)
  QVERIFY(newBookCollection->fields().isEmpty());
}
