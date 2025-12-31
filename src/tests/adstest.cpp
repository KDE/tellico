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

#include "../translators/adsimporter.h"
#include "../fieldformat.h"

#include <KLocalizedString>

#include <QTest>

QTEST_APPLESS_MAIN( AdsTest )

void AdsTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
}

void AdsTest::testImport() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test.ads"));
  QList<QUrl> urls;
  urls << url;
  Tellico::Import::ADSImporter importer(urls);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QCOMPARE(coll->entryCount(), 1);
  QCOMPARE(coll->title(), QStringLiteral("Bibliography"));
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Distant clusters of galaxies detected by X-rays"));
  QCOMPARE(entry->field("entry-type"), QStringLiteral("article"));
  QCOMPARE(entry->field("year"), QStringLiteral("1993"));
  QCOMPARE(entry->field("pages"), QStringLiteral("50-57"));
  const auto authors = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("author")));
  QCOMPARE(authors.count(), 3);
  QCOMPARE(authors.first(), QStringLiteral("Cavaliere, A."));
  QVERIFY(!entry->field("abstract").isEmpty());
  const auto keywords = Tellico::FieldFormat::splitValue(entry->field(QStringLiteral("keyword")));
  QCOMPARE(keywords.count(), 7);
  QCOMPARE(keywords.first(), QStringLiteral("Cosmic Plasma"));
}
