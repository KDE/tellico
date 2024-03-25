/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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

#include "audiofiletest.h"

#include "../translators/audiofileimporter.h"
#include "../images/imagefactory.h"
#include "../images/image.h"

#include <KLocalizedString>

#include <QTest>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( AudioFileTest )

void AudioFileTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::ImageFactory::init();
}

void AudioFileTest::testDirectory() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test.ogg"));
  url = url.adjusted(QUrl::RemoveFilename);
  // url from chooser can be passed to the importer without the trailing slash, see bug 429803
  url = url.adjusted(QUrl::StripTrailingSlash);
  QVERIFY(!url.isEmpty());
  Tellico::Import::AudioFileImporter importer(url);
  importer.setOptions(importer.options() ^ Tellico::Import::ImportProgress);
  importer.setRecursive(true);
  importer.setAddFilePath(true);
  importer.setAddBitrate(true);

  QVERIFY(importer.canImport(Tellico::Data::Collection::Album));
  QVERIFY(!importer.canImport(Tellico::Data::Collection::Book));

  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 2);
  QCOMPARE(coll->title(), QStringLiteral("My Music"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Album"));
  QVERIFY(entry->field("file").contains(QStringLiteral("data/test.ogg")));

  entry = coll->entryById(2);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("mp3 album"));
  QVERIFY(entry->field("file").contains(QStringLiteral("data/audio/test.mp3")));
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  auto img = Tellico::ImageFactory::imageById(entry->field(QStringLiteral("cover")));
  QVERIFY(!img.isNull());
}

void AudioFileTest::testOgg() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test.ogg"));
  QVERIFY(!url.isEmpty());
  Tellico::Import::AudioFileImporter importer(url);
  importer.setOptions(importer.options() ^ Tellico::Import::ImportProgress);
  importer.setAddFilePath(true);
  importer.setAddBitrate(true);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Album"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Album Artist"));
  // test file uses Disc 2
  QVERIFY(entry->field("track").isEmpty());
  QCOMPARE(entry->field("track2"), QStringLiteral("Test OGG::The Artist::0:03"));
  QCOMPARE(entry->field("year"), QStringLiteral("2020"));
  QCOMPARE(entry->field("genre"), QStringLiteral("The Genre"));
  QCOMPARE(entry->field("label"), QStringLiteral("Label"));
  QVERIFY(entry->field("file").contains(QStringLiteral("::160"))); // bitrate
}

void AudioFileTest::testMp3() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/audio/test.mp3"));
  QVERIFY(!url.isEmpty());
  Tellico::Import::AudioFileImporter importer(url);
  importer.setOptions(importer.options() ^ Tellico::Import::ImportProgress);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("mp3 album"));
  QCOMPARE(entry->field("artist"), QStringLiteral("mp3 artist"));
  QCOMPARE(entry->field("track"), QStringLiteral("mp3 title::mp3 artist::0:02"));
  QCOMPARE(entry->field("year"), QStringLiteral("2020"));
  QCOMPARE(entry->field("genre"), QStringLiteral("mp3 genre"));
}

void AudioFileTest::testNonRecursive() {
  // we want the source directory, not the build directory, so look for a source file first
  QFileInfo fi(QFINDTESTDATA("data/test.ogg"));
  QUrl url = QUrl::fromLocalFile(fi.dir().absolutePath());
  QVERIFY(!url.isEmpty());
  Tellico::Import::AudioFileImporter importer(url);
  importer.setOptions(importer.options() ^ Tellico::Import::ImportProgress);
  importer.setRecursive(false);
  importer.setRecursive(true); // check that the bit flipping is correct
  importer.setRecursive(false);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 1);
}
