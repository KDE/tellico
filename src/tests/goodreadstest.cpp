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

#include "goodreadstest.h"

#include "../translators/goodreadsimporter.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../utils/datafileregistry.h"

#include <KConfig>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>
#include <QNetworkInterface>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( GoodreadsTest )

static bool hasNetwork() {
  foreach(const QNetworkInterface& net, QNetworkInterface::allInterfaces()) {
    if(net.flags().testFlag(QNetworkInterface::IsUp) && !net.flags().testFlag(QNetworkInterface::IsLoopBack)) {
      return true;
    }
  }
  return false;
}

void GoodreadsTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/goodreads2tellico.xsl"));
  Tellico::ImageFactory::init();
}

void GoodreadsTest::testImport() {
  if(!hasNetwork()) QSKIP("This test requires network access", SkipSingle);

  Tellico::Import::GoodreadsImporter imp;
  QVERIFY(imp.canImport(Tellico::Data::Collection::Book));
  QVERIFY(!imp.canImport(Tellico::Data::Collection::Album));

  KSharedConfig::Ptr config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig);
  KConfigGroup cg(config, QStringLiteral("ImportOptions - Goodreads"));
  // user name instead of user id to test conversion
  cg.writeEntry("User ID", QStringLiteral("robbystephenson"));
  config->sync();
  imp.setConfig(config);

  Tellico::Data::CollPtr coll(imp.collection());
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Book);

  Tellico::Data::EntryList entries = coll->entries();
  QVERIFY(!entries.isEmpty());
  Tellico::Data::EntryPtr entry = entries.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Diplomatic Immunity"));
  QCOMPARE(entry->field(QStringLiteral("isbn")), QStringLiteral("0-7434-6802-3"));
  QCOMPARE(entry->field(QStringLiteral("pages")), QStringLiteral("367"));
  QCOMPARE(entry->field(QStringLiteral("goodreads")), QStringLiteral("https://www.goodreads.com/book/show/61901.Diplomatic_Immunity"));
  QCOMPARE(entry->field(QStringLiteral("binding")), QStringLiteral("Paperback"));
  QCOMPARE(entry->field(QStringLiteral("publisher")), QStringLiteral("Earthlight"));
  QCOMPARE(entry->field(QStringLiteral("author")), QStringLiteral("Lois McMaster Bujold"));
  QCOMPARE(entry->field(QStringLiteral("pub_year")), QStringLiteral("2003"));
  QCOMPARE(entry->field(QStringLiteral("read")), QStringLiteral("true"));
  QCOMPARE(entry->field(QStringLiteral("rating")), QStringLiteral("3"));

  // TODO: create a description field instead of using comments?
  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));

  // verify user name was converted to user id
  QCOMPARE(cg.readEntry("User ID"), QStringLiteral("3937878"));
}
