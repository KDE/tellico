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

#include "modstest.h"

#include "../translators/xsltimporter.h"
#include "../translators/xslthandler.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../utils/datafileregistry.h"

#include <KLocalizedString>

#include <QTest>
#include <QDomDocument>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QTextCodec>
#else
#include <QStringConverter>
#endif

QTEST_APPLESS_MAIN( ModsTest )

void ModsTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/mods2tellico.xsl"));
}

void ModsTest::testBook() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/example_mods.xml"));
  Tellico::Import::XSLTImporter importer(url);
  importer.setXSLTURL(QUrl::fromLocalFile(QFINDTESTDATA("../../xslt/mods2tellico.xsl")));

  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 1);
  QCOMPARE(coll->title(), QStringLiteral("MODS Import"));

  Tellico::Data::EntryPtr entry = coll->entryById(1);
  QVERIFY(entry);
  QCOMPARE(entry->field("title"), QStringLiteral("Sound and fury"));
  QCOMPARE(entry->field("author"), QStringLiteral("Alterman, Eric"));
  QCOMPARE(entry->field("genre"), QStringLiteral("bibliography"));
  QCOMPARE(entry->field("pub_year"), QStringLiteral("1999"));
  QCOMPARE(entry->field("isbn"), QStringLiteral("0-8014-8639-4"));
  QCOMPARE(entry->field("lccn"), QStringLiteral("99042030"));
}

// The Deutsche Nationalbibliothek SRU server returns MARCXML encapsulated in SRW
// The LoC MARC21 to MODS stylesheet has to be modified to accept multiple mods:record elements
// outside of the marc:collection element
// http://www.dnb.de/DE/Service/DigitaleDienste/SRU/sru_node.html
void ModsTest::testDNBMARCXML() {
  Tellico::XSLTHandler marcHandler(QUrl::fromLocalFile(QFINDTESTDATA("../../xslt/MARC21slim2MODS3.xsl")));
  QVERIFY(marcHandler.isValid());

  QFile f(QFINDTESTDATA("data/dnb-marcxml.xml"));
  QVERIFY(f.exists());
  QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
  QTextStream stream(&f);
  const QString mods = marcHandler.applyStylesheet(stream.readAll());

  Tellico::Import::XSLTImporter importer(mods);
  importer.setXSLTURL(QUrl::fromLocalFile(QFINDTESTDATA("../../xslt/mods2tellico.xsl")));

  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);
  QCOMPARE(coll->entryCount(), 25);
}

void ModsTest::testLocaleEncoding() {
  const auto xslt = QByteArray( \
    "<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">" \
    "  <xsl:output method=\"xml\" version=\"1.0\" encoding=\"windows-1251\"/>" \
    "  <xsl:template match=\"/\"/>" \
    "</xsl:stylesheet>" \
  );
  QDomDocument dom;
  dom.setContent(xslt);
  QVERIFY(!dom.isNull());
  QVERIFY(dom.toString().contains(QLatin1String("windows-1251")));
  Tellico::XSLTHandler::setLocaleEncoding(dom);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QVERIFY(dom.toString().contains(QTextCodec::codecForLocale()->name()));
#else
  QVERIFY(dom.toString().contains(QStringConverter::nameForEncoding(QStringConverter::System)));
#endif
}
