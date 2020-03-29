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
#include "../collections/musiccollection.h"
#include "../fieldformat.h"

#include <QTest>

QTEST_APPLESS_MAIN( AudioFileTest )

void AudioFileTest::testDirectory() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test.ogg"));
  url = url.adjusted(QUrl::RemoveFilename);
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
  QCOMPARE(coll->entryCount(), 1);
  QCOMPARE(coll->title(), QStringLiteral("My Music"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Album"));
  QVERIFY(entry->field("file").contains(QStringLiteral("data/test.ogg")));
  QVERIFY(entry->field("file").contains(QStringLiteral("::610"))); // bitrate
}

void AudioFileTest::testOgg() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test.ogg"));
  QVERIFY(!url.isEmpty());
  Tellico::Import::AudioFileImporter importer(url);
  importer.setOptions(importer.options() ^ Tellico::Import::ImportProgress);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);
  QCOMPARE(coll->entryCount(), 1);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("The Album"));
  QCOMPARE(entry->field("artist"), QStringLiteral("Album Artist"));
  QCOMPARE(entry->field("track"), QStringLiteral("Test OGG::The Artist"));
  QCOMPARE(entry->field("year"), QStringLiteral("2020"));
  QCOMPARE(entry->field("genre"), QStringLiteral("The Genre"));
}
