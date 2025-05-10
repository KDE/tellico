/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>
#include "discogstest.h"

#include "../translators/discogsimporter.h"
#include "../collections/musiccollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"

#include <KConfig>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QTest>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QLineEdit>

QTEST_MAIN( DiscogsTest )

static bool hasNetwork() {
#ifdef ENABLE_NETWORK_TESTS
  foreach(const QNetworkInterface& net, QNetworkInterface::allInterfaces()) {
    if(net.flags().testFlag(QNetworkInterface::IsUp) && !net.flags().testFlag(QNetworkInterface::IsLoopBack)) {
      return true;
    }
  }
#endif
  return false;
}

void DiscogsTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  Tellico::ImageFactory::init();
}

void DiscogsTest::testImport() {
  if(!hasNetwork()) QSKIP("This test requires network access", SkipSingle);

  Tellico::Import::DiscogsImporter imp;
  QVERIFY(!imp.canImport(Tellico::Data::Collection::Book));
  QVERIFY(imp.canImport(Tellico::Data::Collection::Album));

  KSharedConfig::Ptr config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig);
  KConfigGroup cg(config, QStringLiteral("ImportOptions - Discogs"));
  cg.writeEntry("User ID", QStringLiteral("tellico-robby"));
  // need to add token to get images
  config->sync();
  imp.setConfig(config);

  Tellico::Data::CollPtr coll(imp.collection());
  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Album);

  Tellico::Data::EntryList entries = coll->entries();
  QVERIFY(!entries.isEmpty());
  Tellico::Data::EntryPtr entry = entries.at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("title")), QStringLiteral("Les Misérables"));
  QCOMPARE(entry->field(QStringLiteral("year")), QStringLiteral("1985"));
  QCOMPARE(entry->field(QStringLiteral("medium")), QStringLiteral("Compact Disc"));
  QCOMPARE(entry->field(QStringLiteral("label")), QStringLiteral("First Night Records"));
  QCOMPARE(entry->field(QStringLiteral("genre")), QStringLiteral("Musical"));
  QCOMPARE(entry->field(QStringLiteral("artist")), QStringLiteral("Alain Boublil; Claude-Michel Schönberg"));
//  QCOMPARE(entry->field(QStringLiteral("rating")), QStringLiteral("4"));

//  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
//  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
//  QVERIFY(!entry->field(QStringLiteral("comments")).isEmpty());
}

void DiscogsTest::testWidget() {
  Tellico::Import::DiscogsImporter importer;

  KSharedConfig::Ptr config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig);
  KConfigGroup cg(config, QStringLiteral("ImportOptions - Discogs"));
  cg.writeEntry("User ID", QStringLiteral("tellico-robby"));
  // need to add token to get images
  config->sync();
  importer.setConfig(config);

  QScopedPointer<QWidget> widget(importer.widget(nullptr));
  QVERIFY(widget);
  auto edits = widget->findChildren<QLineEdit *>();
  QCOMPARE(edits.size(), 2);
  auto edit1 = static_cast<QLineEdit*>(edits[0]);
  QCOMPARE(edit1->text(), QLatin1String("tellico-robby"));
}
