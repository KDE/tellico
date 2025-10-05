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
#include "../utils/datafileregistry.h"
#include "../entryview.h"

#include <KLocalizedString>

#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QStandardPaths>
#include <QLoggingCategory>

QTEST_MAIN( DocumentTest )

void DocumentTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QLoggingCategory::setFilterRules(QStringLiteral("tellico.debug = true\ntellico.info = false"));
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::ImageFactory::init();
  // test case is a book file
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/"));
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
  QString imageName = QStringLiteral("17b54b2a742c6d342a75f122d615a793.jpeg");

  // copy a collection file that includes an image into the temporary directory
  QVERIFY(QFile::copy(QFINDTESTDATA("data/with-image.tc"), fileName));

  auto doc = Tellico::Data::Document::self();
  QVERIFY(doc->openDocument(QUrl::fromLocalFile(fileName)));
  QCOMPARE(Tellico::ImageFactory::localDir(), QUrl::fromLocalFile(imageDirName));

  Tellico::Data::CollPtr coll = doc->collection();
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->title(), QStringLiteral("My Books"));
  QCOMPARE(coll->entries().size(), 1);

  Tellico::Data::EntryPtr e = coll->entries().at(0);
  QVERIFY(e);
  QCOMPARE(e->field(QStringLiteral("cover")), imageName);

  // save the document, so the images get copied out of the .tc file into the local image directory
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName)));
  // verify that backup file gets created
  QVERIFY(QFile::exists(fileName + '~'));

  // check that the local image directory is created with the image file inside
  QDir imageDir(imageDirName);
  QVERIFY(imageDir.exists());
  QCOMPARE(e->field(QStringLiteral("cover")), imageName);
  QVERIFY(imageDir.exists(imageName));

  // clear the internal image cache
  Tellico::ImageFactory::clean(true);

  // verify that the images are copied from the old directory when saving to a new file
  QString fileName2 = tempDirName + "/with-image2.tc";
  QString imageDirName2 = tempDirName + "/with-image2_files/";
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName2)));
  QVERIFY(QFile::exists(fileName2));
  QDir imageDir2(imageDirName2);
  QVERIFY(imageDir2.exists());
  QVERIFY(imageDir2.exists(imageName));

  // removing the image from the entry should result in it no longer being in the dir
  // https://bugs.kde.org/show_bug.cgi?id=509244
  e->setField(QStringLiteral("cover"), QString());
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName2)));
  QCOMPARE(imageDir2.exists(imageName), false);

  // should still be in image cache in memory
  e->setField(QStringLiteral("cover"), imageName);
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName2)));
  QVERIFY(imageDir2.exists(imageName));

  // add additional entry with same image
  Tellico::Data::EntryPtr e2(new Tellico::Data::Entry(*e));
  coll->addEntries(e2);
  e->setField(QStringLiteral("cover"), QString());
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName2)));
  QVERIFY(imageDir2.exists(imageName));  // since image is in 2 entries, should still exist
  // remove all the entries and save, the image should be gone again
  coll->removeEntries(coll->entries());
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName2)));
  QCOMPARE(imageDir2.exists(imageName), false);

  /***********************************************************************/
  /* now also verify image directory when file name has multiple periods */
  /* see https://bugs.kde.org/show_bug.cgi?id=348088                     */
  /* also have to check backwards compatibility with prior behavior      */
  /***********************************************************************/

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
  QVERIFY(!imageDir3.exists(imageName));
  QVERIFY(!Tellico::ImageFactory::imageById(imageName).isNull());

  // now remove the first image from the first image directory, save the document, and verify that
  // the proper image exists and is written
  QVERIFY(imageDir.remove(imageName));
  QVERIFY(!imageDir.exists(imageName));
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName3)));
  // now the file should exist in the proper location
  QVERIFY(imageDir3.exists(imageName));
  // clear the cache
  Tellico::ImageFactory::clean(true);
  QVERIFY(!Tellico::ImageFactory::imageById(imageName).isNull());

  // now create a new collection and check the image location
  // should be changed back to a temporary dir
  QVERIFY(doc->newDocument(Tellico::Data::Collection::Book));
  QCOMPARE(Tellico::ImageFactory::cacheDir(), Tellico::ImageFactory::TempDir);

  // sanity check, the directory should not exist after QTemporaryDir destruction
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

void DocumentTest::testView() {
  Tellico::EntryView view(nullptr);
  view.setXSLTFile(QLatin1String("Fancy.xsl"));
  // testing the configuration for the main window
  view.setUseImageConfigLocation(true);

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
  QDir imageDir(imageDirName);
  QCOMPARE(imageDir.exists(), false);
  const QString imageName = QStringLiteral("17b54b2a742c6d342a75f122d615a793.jpeg");

  // copy a collection file that includes an image into the temporary directory
  QVERIFY(QFile::copy(QFINDTESTDATA("data/with-image.tc"), fileName));

  auto doc = Tellico::Data::Document::self();
  QVERIFY(doc->openDocument(QUrl::fromLocalFile(fileName)));
  QCOMPARE(Tellico::ImageFactory::localDir(), QUrl::fromLocalFile(imageDirName));
  // image folder still does not exist yet
  QCOMPARE(imageDir.exists(), false);

  Tellico::Data::CollPtr coll = doc->collection();
  QVERIFY(coll);

  Tellico::Data::EntryPtr e = coll->entries().at(0);
  QVERIFY(e);
  QCOMPARE(e->field(QStringLiteral("cover")), imageName);

  view.showEntry(e);
  // now it exists
  QVERIFY(imageDir.exists());
  QVERIFY(imageDir.exists(imageName));

  // Bug 508902
  // Search for an entry, show it in a second view (like the FetchDialog does)
  Tellico::Data::CollPtr newColl(new Tellico::Data::BookCollection(true));
  Tellico::Data::EntryPtr newEntry(new Tellico::Data::Entry(newColl));
  newEntry->setField(QLatin1String("title"), QLatin1String("new title"));
  const QUrl imageUrl = QUrl::fromLocalFile(QFINDTESTDATA("../../icons/tellico.png"));
  const auto newImageId = Tellico::ImageFactory::addImage(imageUrl, true);
  newEntry->setField(QLatin1String("cover"), newImageId);

  Tellico::EntryView view2(nullptr);
  view2.setXSLTFile(QLatin1String("Fancy.xsl"));
  view2.showEntry(newEntry);
  // new image should not exist in the data dir
  QCOMPARE(imageDir.exists(newImageId), false);

  // now add it to collection, but don't save the document yet
  // follows what AddEntries does
  Tellico::Data::EntryList list;
  list.append(Tellico::Data::EntryPtr(new Tellico::Data::Entry(*newEntry)));
  coll->addEntries(list);

  // then show it in original view
  view.showEntry(list.front());
  // it exists in the data dir even though it's not saved to the document
  QVERIFY(imageDir.exists(newImageId));
}
