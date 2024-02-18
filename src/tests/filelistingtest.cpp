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

#include <config.h>
#include "filelistingtest.h"

#include "../translators/filelistingimporter.h"
#include "../translators/xmphandler.h"
#include "../images/imagefactory.h"
#include "../core/netaccess.h"

#include <KLocalizedString>

#include <QTest>
#include <QLoggingCategory>
#include <QStandardPaths>

// KIO::listDir in FileListingImporter seems to require a GUI Application
QTEST_MAIN( FileListingTest )

void FileListingTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::ImageFactory::init();
  QLoggingCategory::setFilterRules(QStringLiteral("tellico.debug = true\ntellico.info = true"));
}

void FileListingTest::testCpp() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("filelistingtest.cpp"));
  Tellico::Import::FileListingImporter importer(url.adjusted(QUrl::RemoveFilename));
  QVERIFY(importer.canImport(Tellico::Data::Collection::File));
  // can't import images for local test
//  importer.setOptions(importer.options() & ~Tellico::Import::ImportShowImageErrors);
  importer.setUseFilePreview(true);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::File);
  QVERIFY(coll->entryCount() > 0);

  Tellico::Data::EntryPtr entry;
  foreach(Tellico::Data::EntryPtr tmpEntry, coll->entries()) {
    if(tmpEntry->field(QStringLiteral("title")) == QStringLiteral("filelistingtest.cpp")) {
      entry = tmpEntry;
    }
  }
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("filelistingtest.cpp"));
  QCOMPARE(entry->field("url"), url.url());
  QVERIFY(entry->field("description").contains("C++"));
  QCOMPARE(entry->field("folder"), QString()); // empty relative folder location
  QCOMPARE(entry->field("mimetype"), QStringLiteral("text/x-c++src"));
  QVERIFY(!entry->field("size").isEmpty());
  QVERIFY(!entry->field("permissions").isEmpty());
  QVERIFY(!entry->field("owner").isEmpty());
  QVERIFY(!entry->field("group").isEmpty());
  // for some reason, the Creation time isn't populated for this test
//  QVERIFY(!entry->field("created").isEmpty());
  QVERIFY(!entry->field("modified").isEmpty());
  QCOMPARE(entry->field("metainfo"), QString());
  QVERIFY(!entry->field("icon").isEmpty());
}

void FileListingTest::testXMPData() {
  {
    Tellico::XMPHandler xmp;
#ifdef HAVE_EXEMPI
    QVERIFY(xmp.isXMPEnabled());
    QVERIFY(!xmp.extractXMP(QFINDTESTDATA("data/BlueSquare.jpg")).isEmpty());
#endif
  }
  // initializing exempi can cause a crash in Exiv for files with XMP data
  // see https://bugs.kde.org/show_bug.cgi?id=390744
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/BlueSquare.jpg"));
  Tellico::Import::FileListingImporter importer(url.adjusted(QUrl::RemoveFilename));
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::File);
  QVERIFY(coll->entryCount() > 0);

  Tellico::Data::EntryPtr entry, oggEntry;
  foreach(Tellico::Data::EntryPtr tmpEntry, coll->entries()) {
    if(tmpEntry->field(QStringLiteral("title")) == QStringLiteral("BlueSquare.jpg")) {
      entry = tmpEntry;
    } else if(tmpEntry->field(QStringLiteral("title")) == QStringLiteral("test.ogg")) {
      oggEntry = tmpEntry;
    }
  }
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("BlueSquare.jpg"));
  QCOMPARE(entry->field("mimetype"), QStringLiteral("image/jpeg"));
  QCOMPARE(entry->field("size"), QStringLiteral("23.6 KiB"));
#ifdef HAVE_KFILEMETADATA
#ifdef HAVE_EXEMPI
  QEXPECT_FAIL("", "Because of a crash related to exempi and kfilemetadata linking, no metadata is read", Continue);
#endif
  QVERIFY(!entry->field("metainfo").isEmpty());
#endif

  QVERIFY(oggEntry);
  QCOMPARE(oggEntry->field("title"), QStringLiteral("test.ogg"));
  QCOMPARE(oggEntry->field("mimetype"), QStringLiteral("audio/x-vorbis+ogg"));
  QStringList meta = Tellico::FieldFormat::splitTable(oggEntry->field(QStringLiteral("metainfo")));
  QVERIFY(!meta.isEmpty());
  QVERIFY(meta.contains(QStringLiteral("Bitrate::159000")));
}

void FileListingTest::testStat() {
  QUrl local = QUrl::fromLocalFile(QFINDTESTDATA("filelistingtest.cpp"));
  QVERIFY(Tellico::NetAccess::exists(local, false, nullptr));
  QUrl remote(QStringLiteral("https://tellico-project.org"));
  // http doesn't support existence without downloading
  QVERIFY(!Tellico::NetAccess::exists(remote, false, nullptr));
}

void FileListingTest::testVideo() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test_movie.mpg"));
  Tellico::Import::FileListingImporter importer(url.adjusted(QUrl::RemoveFilename));
  importer.setCollectionType(Tellico::Data::Collection::Video);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 1);

  auto entry = coll->entries().front();
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Test Movie"));
  QCOMPARE(entry->field("origtitle"), QStringLiteral("The original title"));
  QCOMPARE(entry->field("imdb"), QStringLiteral("https://www.imdb.com/title/tt0012345"));
  QCOMPARE(entry->field("tmdb"), QStringLiteral("https://www.themoviedb.org/movie/345"));
  QCOMPARE(entry->field("year"), QStringLiteral("2004"));
  QCOMPARE(entry->field("running-time"), QStringLiteral("113"));
  QCOMPARE(entry->field("certification"), QStringLiteral("PG (USA)"));
  QCOMPARE(entry->field("genre"), QStringLiteral("Science Fiction; Romance"));
  QCOMPARE(entry->field("keyword"), QStringLiteral("Favorite"));
  QCOMPARE(entry->field("nationality"), QStringLiteral("USA"));
  QCOMPARE(entry->field("studio"), QStringLiteral("Paramount"));
  QCOMPARE(entry->field("writer"), QStringLiteral("Jill W. Writer"));
  QCOMPARE(entry->field("director"), QStringLiteral("John B. Director; Famous Director"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QStringLiteral("Jill Actress::Heroine"));
  QVERIFY(!entry->field("plot").isEmpty());
}
