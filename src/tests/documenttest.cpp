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

#include "documenttest.h"
#include "documenttest.moc"
#include "qtest_kde.h"

#include "../images/imagefactory.h"
#include "../core/tellico_config.h"
#include "../document.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"

#include <KTempDir>

#include <QFile>

QTEST_KDEMAIN_CORE( DocumentTest )

void DocumentTest::initTestCase() {
  Tellico::ImageFactory::init();
  // test case is a book file
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
}

void DocumentTest::testImageLocalDirectory() {
  Tellico::Config::setImageLocation(Tellico::Config::ImagesInLocalDir);
  // the default collection will use a temporary directory as a local image dir
  QVERIFY(!Tellico::ImageFactory::localDir().isEmpty());

  QString tempDirName;

  KTempDir tempDir;
  tempDir.setAutoRemove(true);
  tempDirName = tempDir.name();
  QString fileName = tempDirName + "with-image.tc";
  QString imageDirName = tempDirName + "with-image_files/";

  // copy a collection file that includes an image into the temporary directory
  QVERIFY(QFile::copy(QString::fromLatin1(KDESRCDIR "/data/with-image.tc"),
                      fileName));

  Tellico::Data::Document* doc = Tellico::Data::Document::self();
  QVERIFY(doc->openDocument(fileName));
  QCOMPARE(Tellico::ImageFactory::localDir(), imageDirName);

  Tellico::Data::CollPtr coll = doc->collection();
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->title(), QLatin1String("My Books"));
  QCOMPARE(coll->entries().size(), 1);

  Tellico::Data::EntryPtr e = coll->entries().at(0);
  QVERIFY(e);
  QCOMPARE(e->field(QLatin1String("cover")), QLatin1String("17b54b2a742c6d342a75f122d615a793.jpeg"));

  // save the document, so the images get copied out of the .tc file into the local image directory
  QVERIFY(doc->saveDocument(fileName));
  // verify that backup file gets created
  QVERIFY(QFile::exists(fileName + '~'));

  // check that the local image directory is created with the image file inside
  QDir imageDir(imageDirName);
  QVERIFY(imageDir.exists());
  QVERIFY(imageDir.exists(e->field(QLatin1String("cover"))));

  // clear the internal image cache
  Tellico::ImageFactory::clean(true);

  /*************************************************************************/
  /* now also verify image directory when file name has multiple periods */
  /* see https://bugs.kde.org/show_bug.cgi?id=348088 */
  /* also have to check backwards compatability with prior behavior */
  /*************************************************************************/

  QString fileName2 = tempDirName + "with-image.1.tc";
  QString imageDirName2 = tempDirName + "with-image.1_files/";

  // copy the collection file, which no longer contains the images inside
  QVERIFY(QFile::copy(fileName, fileName2));
  QVERIFY(doc->openDocument(fileName2));
  QCOMPARE(Tellico::ImageFactory::localDir(), imageDirName2);
  QDir imageDir2(imageDirName2);

  // verify that the images can be loaded from the image directory that does NOT have multiple periods
  // since that was the behavior prior to the bug being fixed
  coll = doc->collection();
  e = coll->entries().at(0);
  // image should not be in the next image dir yet since we haven't saved
  QVERIFY(!imageDir2.exists(e->field(QLatin1String("cover"))));
  QVERIFY(!Tellico::ImageFactory::imageById(e->field("cover")).isNull());

  // now remove the first image from the first image directory, save the document, and verify that
  // the proper image exists and is written
  QVERIFY(imageDir.remove(e->field("cover")));
  QVERIFY(!imageDir.exists(e->field(QLatin1String("cover"))));
  QVERIFY(doc->saveDocument(fileName2));
  // now the file should exist in the proper location
  QVERIFY(imageDir2.exists(e->field(QLatin1String("cover"))));
  // clear the cache
  Tellico::ImageFactory::clean(true);
  QVERIFY(!Tellico::ImageFactory::imageById(e->field("cover")).isNull());

  // sanity check, the directory should not exists after KTempDir destruction
  tempDir.unlink();
  QVERIFY(!QDir(tempDirName).exists());
}
