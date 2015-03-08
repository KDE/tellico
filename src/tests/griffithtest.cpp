/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "griffithtest.h"
#include "qtest_kde.h"

#include "../translators/griffithimporter.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"

#include <kstandarddirs.h>

#define FIELDS(entry, fieldName) Tellico::FieldFormat::splitValue(entry->field(fieldName))
#define ROWS(entry, fieldName) Tellico::FieldFormat::splitTable(entry->field(fieldName))

QTEST_KDEMAIN_CORE( GriffithTest )

void GriffithTest::initTestCase() {
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();
  // need to register the collection type
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
}

void GriffithTest::testMovies() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/griffith.xml");
  Tellico::Import::GriffithImporter importer(url);
  // can't import images for local test
  importer.setOptions(importer.options() & ~Tellico::Import::ImportShowImageErrors);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Video);
  QCOMPARE(coll->entryCount(), 5);

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("Serendipity"));
  QCOMPARE(entry->field("origtitle"), QLatin1String("Serendipity"));
  QCOMPARE(entry->field("director"), QLatin1String("Peter Chelsom"));
  QCOMPARE(entry->field("year"), QLatin1String("2001"));
  QCOMPARE(entry->field("certification"), QLatin1String("PG-13 (USA)"));
  QCOMPARE(entry->field("nationality"), QLatin1String("USA"));
  QCOMPARE(entry->field("genre"), QLatin1String("Comedy; Romance; Fantasy"));
  QCOMPARE(entry->field("rating"), QLatin1String("6"));
  QCOMPARE(entry->field("running-time"), QLatin1String("90"));
  QCOMPARE(entry->field("studio"), QLatin1String("studio"));
  QCOMPARE(entry->field("seen"), QLatin1String("true"));
  QCOMPARE(entry->field("medium"), QLatin1String("DVD"));
  QCOMPARE(ROWS(entry, "cast").first(), QLatin1String("John Cusack::Jonathan Trager"));
  QVERIFY(!entry->field("plot").isEmpty());
  // cover will be empty since local images don't exist
}
