/***************************************************************************
    Copyright (C) 2015-2022 Robby Stephenson <robby@periapsis.org>
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

#include "documenttest.h"
#include "../document.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../config/tellico_config.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"

#include <KLocalizedString>

#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( DocumentTest )

void DocumentTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::ImageFactory::init();
  // test case is a book file
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
}

void DocumentTest::cleanupTestCase() {
  Tellico::ImageFactory::clean(true);
}

void DocumentTest::testImageLocalDirectory() {
  Tellico::Config::setImageLocation(Tellico::Config::ImagesInLocalDir);
  // the default collection will use a temporary directory as a local image dir
  QVERIFY(!Tellico::ImageFactory::localDir().isEmpty());

  QString tempDirName;

  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  tempDir.setAutoRemove(true);
  tempDirName = tempDir.path();
  QString fileName = tempDirName + "/with-image.tc";
  QString imageDirName = tempDirName + "/with-image_files/";

  // copy a collection file that includes an image into the temporary directory
  QVERIFY(QFile::copy(QFINDTESTDATA("data/with-image.tc"), fileName));

  Tellico::Data::Document* doc = Tellico::Data::Document::self();
  QVERIFY(doc->openDocument(QUrl::fromLocalFile(fileName)));
  QCOMPARE(Tellico::ImageFactory::localDir(), QUrl::fromLocalFile(imageDirName));

  Tellico::Data::CollPtr coll = doc->collection();
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->title(), QStringLiteral("My Books"));
  QCOMPARE(coll->entries().size(), 1);

  Tellico::Data::EntryPtr e = coll->entries().at(0);
  QVERIFY(e);
  QCOMPARE(e->field(QStringLiteral("cover")), QStringLiteral("17b54b2a742c6d342a75f122d615a793.jpeg"));

  // save the document, so the images get copied out of the .tc file into the local image directory
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName)));
  // verify that backup file gets created
  QVERIFY(QFile::exists(fileName + '~'));

  // check that the local image directory is created with the image file inside
  QDir imageDir(imageDirName);
  QVERIFY(imageDir.exists());
  QVERIFY(imageDir.exists(e->field(QStringLiteral("cover"))));

  // clear the internal image cache
  Tellico::ImageFactory::clean(true);

  // verify that the images are copied from the old directory when saving to a new file
  QString fileName2 = tempDirName + "/with-image2.tc";
  QString imageDirName2 = tempDirName + "/with-image2_files/";
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName2)));
  QVERIFY(QFile::exists(fileName2));
  QDir imageDir2(imageDirName2);
  QVERIFY(imageDir2.exists());
  QVERIFY(imageDir2.exists(e->field(QStringLiteral("cover"))));

  /*************************************************************************/
  /* now also verify image directory when file name has multiple periods */
  /* see https://bugs.kde.org/show_bug.cgi?id=348088 */
  /* also have to check backwards compatibility with prior behavior */
  /*************************************************************************/

  QString fileName3 = tempDirName + "/with-image.1.tc";
  QString imageDirName3 = tempDirName + "/with-image.1_files/";

  // copy the collection file, which no longer contains the images inside
  QVERIFY(QFile::copy(fileName, fileName3));
  QVERIFY(doc->openDocument(QUrl::fromLocalFile(fileName3)));
  QCOMPARE(Tellico::ImageFactory::localDir(), QUrl::fromLocalFile(imageDirName3));
  QDir imageDir3(imageDirName3);

  // verify that the images can be loaded from the image directory that does NOT have multiple periods
  // since that was the behavior prior to the bug being fixed
  coll = doc->collection();
  e = coll->entries().at(0);
  // image should not be in the next image dir yet since we haven't saved
  QVERIFY(!imageDir3.exists(e->field(QStringLiteral("cover"))));
  QVERIFY(!Tellico::ImageFactory::imageById(e->field("cover")).isNull());

  // now remove the first image from the first image directory, save the document, and verify that
  // the proper image exists and is written
  QVERIFY(imageDir.remove(e->field("cover")));
  QVERIFY(!imageDir.exists(e->field(QStringLiteral("cover"))));
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName3)));
  // now the file should exist in the proper location
  QVERIFY(imageDir3.exists(e->field(QStringLiteral("cover"))));
  // clear the cache
  Tellico::ImageFactory::clean(true);
  QVERIFY(!Tellico::ImageFactory::imageById(e->field("cover")).isNull());

  // sanity check, the directory should not exists after QTemporaryDir destruction
  tempDir.remove();
  QVERIFY(!QDir(tempDirName).exists());
}

void DocumentTest::testSaveTemplate() {
  auto doc = Tellico::Data::Document::self();
  QVERIFY(doc);
  QVERIFY(doc->newDocument(Tellico::Data::Collection::Book));
  auto coll = doc->collection();
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  coll->addEntries(entry1);
  entry1->setField(QStringLiteral("title"), QStringLiteral("new title"));
  // modify a field too, to check that the template saves the modified field
  auto field = coll->fieldByName(QStringLiteral("publisher"));
  field->setTitle(QStringLiteral("batman"));

  Tellico::FilterRule* rule1 = new Tellico::FilterRule(QStringLiteral("title"),
                                                       QStringLiteral("Star Wars"),
                                                       Tellico::FilterRule::FuncEquals);
  Tellico::FilterPtr filter(new Tellico::Filter(Tellico::Filter::MatchAny));
  filter->append(rule1);
  coll->addFilter(filter);

  QString templateName(QStringLiteral("my new template"));
  QTemporaryFile templateFile(QStringLiteral("documenttest-template.XXXXXX.xml"));
  QVERIFY(templateFile.open());
  QUrl templateUrl = QUrl::fromLocalFile(templateFile.fileName());
  QVERIFY(doc->saveDocumentTemplate(templateUrl, templateName));

  QVERIFY(doc->openDocument(templateUrl));
  auto new_coll = doc->collection();
  QVERIFY(new_coll);
  QCOMPARE(new_coll->title(), templateName);
  QCOMPARE(new_coll->entryCount(), 0);

  auto new_field = new_coll->fieldByName(QStringLiteral("publisher"));
  QCOMPARE(new_field->title(), QStringLiteral("batman"));

  QCOMPARE(new_coll->filters().count(), 1);
}
