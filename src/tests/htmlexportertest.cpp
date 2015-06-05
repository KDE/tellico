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

#include "htmlexportertest.h"

#include "../translators/htmlexporter.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <QRegExp>
#include <QTest>

QTEST_GUILESS_MAIN( HtmlExporterTest )

void HtmlExporterTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/tellico2html.xsl"));
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/report-templates/Column_View.xsl"));
}

void HtmlExporterTest::testHtml() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true));
  coll->setTitle(QLatin1String("Robby's Books"));

  Tellico::Data::EntryPtr e(new Tellico::Data::Entry(coll));
  coll->addEntries(e);

  Tellico::Export::HTMLExporter exporter(coll);
  exporter.setEntries(coll->entries());

  QString output = exporter.text();
//  qDebug() << output;
  QVERIFY(!output.isEmpty());

  // check https://bugs.kde.org/show_bug.cgi?id=348381
  QRegExp rx("<title>.*</title>");
  rx.setMinimal(true);
  QVERIFY(output.contains(rx));
  QCOMPARE(rx.cap(), QLatin1String("<title>Robby's Books</title>"));
}

void HtmlExporterTest::testReportHtml() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true));
  coll->setTitle(QLatin1String("Robby's Books"));

  Tellico::Data::EntryPtr e(new Tellico::Data::Entry(coll));
  coll->addEntries(e);

  Tellico::Export::HTMLExporter exporter(coll);
  exporter.setXSLTFile(QFINDTESTDATA("../../xslt/report-templates/Column_View.xsl"));
  exporter.setEntries(coll->entries());

  QString output = exporter.text();
//  qDebug() << output;
  QVERIFY(!output.isEmpty());

  // check that cdate is passed correctly
  QRegExp rx("<p id=\"header-right\">(.*)</p>");
  rx.setMinimal(true);
  QVERIFY(output.contains(rx));
  QCOMPARE(rx.cap(1), QLocale().toString(QDate::currentDate()));
}
