/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "adstest.h"
#include "adstest.moc"
#include "qtest_kde.h"

#include "../translators/adsimporter.h"
#include "../collections/bibtexcollection.h"
#include "../fieldformat.h"

QTEST_KDEMAIN_CORE( AdsTest )

void AdsTest::testImport() {
  KUrl url(QString::fromLatin1(KDESRCDIR) + "/data/test.ads");
  KUrl::List urls;
  urls << url;
  Tellico::Import::ADSImporter importer(urls);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll.isNull());
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 1);
  QCOMPARE(coll->title(), QLatin1String("Bibliography"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(!entry.isNull());
  QCOMPARE(entry->field("title"), QLatin1String("Distant clusters of galaxies detected by X-rays"));
  QCOMPARE(entry->field("entry-type"), QLatin1String("article"));
  QCOMPARE(entry->field("year"), QLatin1String("1993"));
  QCOMPARE(entry->field("pages"), QLatin1String("50-57"));
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field("author")).count(), 3);
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field("author")).first(), QLatin1String("Cavaliere, A."));
  QVERIFY(!entry->field("abstract").isEmpty());
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field("keyword")).count(), 7);
  QCOMPARE(Tellico::FieldFormat::splitValue(entry->field("keyword")).first(), QLatin1String("Cosmic Plasma"));
}
