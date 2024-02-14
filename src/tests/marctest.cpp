/***************************************************************************
    Copyright (C) 2022 Robby Stephenson <robby@periapsis.org>
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

#include "marctest.h"

#include "../translators/marcimporter.h"
#include "../collections/bibtexcollection.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../utils/datafileregistry.h"

#include <KLocalizedString>
#include <KComboBox>

#include <QTest>
#include <QStandardPaths>

QTEST_MAIN( MarcTest )

void MarcTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/mods2tellico.xsl"));
}

void MarcTest::testMarc() {
  const QString marcdump = QStandardPaths::findExecutable(QStringLiteral("yaz-marcdump"));
  if(marcdump.isEmpty()) {
    QSKIP("This test requires yaz-marcdump", SkipAll);
  }
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test.mrc"));
  Tellico::Import::MarcImporter importer(url);
  importer.setCharacterSet(QStringLiteral("iso-8859-1"));

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 1);
  // since the importer uses MODS as an intermediate format, the title reflects that
  QCOMPARE(coll->title(), QStringLiteral("MODS Import"));
  QVERIFY(importer.canImport(coll->type()));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QString::fromUtf8("Det osynliga barnet och andra berättelser"));
  QCOMPARE(entry->field("author"), QStringLiteral("Jansson, Tove"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("1998"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("951-500880-8"));
}

void MarcTest::testWidget() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/test.mrc"));
  Tellico::Import::MarcImporter importer(url);
  QScopedPointer<QWidget> widget(importer.widget(nullptr));
  QVERIFY(widget);

  auto boxes = widget->findChildren<KComboBox *>();
  QCOMPARE(boxes.size(), 2);
  auto box1 = static_cast<KComboBox*>(boxes[0]);
  QCOMPARE(box1->itemText(0), QLatin1String("UTF-8"));
  auto box2 = static_cast<KComboBox*>(boxes[1]);
  QCOMPARE(box2->itemText(0), QLatin1String("MARC21"));

  importer.setCharacterSet(QStringLiteral("iso-8859-1"));
  QCOMPARE(box1->currentText(), QLatin1String("iso-8859-1"));
}
