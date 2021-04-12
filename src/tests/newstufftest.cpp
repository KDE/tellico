/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "newstufftest.h"
#include "../newstuff/manager.h"
#include "../utils/tellico_utils.h"

#include <QTest>
#include <QStandardPaths>
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusInterface>

QTEST_MAIN( NewStuffTest )

void NewStuffTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  // start with a clean test directory
  QDir dir(Tellico::saveLocation(QStringLiteral("entry-templates/")));
  QVERIFY(dir.removeRecursively());
  dir.setPath(Tellico::saveLocation(QStringLiteral("data-sources/")));
  QVERIFY(dir.removeRecursively());
  Tellico::NewStuff::Manager::self();
}

void NewStuffTest::testConnection() {
  auto conn = QDBusConnection::sessionBus();
  QVERIFY(conn.isConnected());
  auto obj = conn.objectRegisteredAt(QStringLiteral("/NewStuff"));
  QVERIFY(obj);

  auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.tellico"),          // Service
                                            QStringLiteral("/NewStuff"),                // Path
                                            QStringLiteral("org.kde.tellico.newstuff"), // Interface
                                            QStringLiteral("installTemplate")           // Method
                                           );
  msg.setArguments(QVariantList() << QString()); // one arg, file to install
  QDBusReply<bool> reply = conn.call(msg);
  QVERIFY(reply.isValid());
  QCOMPARE(reply.value(), false); // false because of empty installation file
}

void NewStuffTest::testNewStuff() {
  auto list = Tellico::NewStuff::Manager::self()->userTemplates();
  QVERIFY(list.isEmpty());
}

void NewStuffTest::testInstallTemplate() {
  auto startList = Tellico::NewStuff::Manager::self()->userTemplates();

  const QString templateFile = QFINDTESTDATA("../../xslt/entry-templates/Compact.xsl");
  QVERIFY(Tellico::NewStuff::Manager::self()->installTemplate(templateFile));
  auto listAfterInstall = Tellico::NewStuff::Manager::self()->userTemplates();
  QCOMPARE(startList.count(), listAfterInstall.count()-1); // one more file got installed
  QVERIFY(listAfterInstall.contains(QStringLiteral("Compact")));

  QVERIFY(Tellico::NewStuff::Manager::self()->removeTemplate(templateFile));
  auto listAfterDelete = Tellico::NewStuff::Manager::self()->userTemplates();
  QCOMPARE(startList.count(), listAfterDelete.count()); // one more file got installed
  QVERIFY(!listAfterDelete.contains(QStringLiteral("Compact")));

  QVERIFY(Tellico::NewStuff::Manager::self()->installTemplate(templateFile));
  listAfterInstall = Tellico::NewStuff::Manager::self()->userTemplates();
  QVERIFY(Tellico::NewStuff::Manager::self()->removeTemplateByName(QStringLiteral("Compact")));

  QDBusInterface iface(QStringLiteral("org.kde.tellico"), QStringLiteral("/NewStuff"), QStringLiteral("org.kde.tellico.newstuff"));
  QDBusReply<bool> reply = iface.call(QStringLiteral("installTemplate"), templateFile);
  QVERIFY(reply.isValid());
  QCOMPARE(reply.value(), true);
  listAfterInstall = Tellico::NewStuff::Manager::self()->userTemplates();
  QVERIFY(listAfterInstall.contains(QStringLiteral("Compact")));

  reply = iface.call(QStringLiteral("removeTemplate"), templateFile);
  QVERIFY(reply.isValid());
  QCOMPARE(reply.value(), true);
  listAfterDelete = Tellico::NewStuff::Manager::self()->userTemplates();
  QVERIFY(!listAfterDelete.contains(QStringLiteral("Compact")));
}

void NewStuffTest::testInstallScript() {
  QDir dir(Tellico::saveLocation(QStringLiteral("data-sources/dark_horse_comics/")));
  QVERIFY(!dir.exists(QStringLiteral("dark_horse_comics.py")));

  const QString scriptFile = QFINDTESTDATA("../fetch/scripts/dark_horse_comics.py");
  QVERIFY(Tellico::NewStuff::Manager::self()->installScript(scriptFile));
  QVERIFY(dir.exists(QStringLiteral("dark_horse_comics.py")));

  QVERIFY(Tellico::NewStuff::Manager::self()->removeScriptByName(QStringLiteral("dark_horse_comics")));
  QVERIFY(!dir.exists()); // complete directory should not exist

  QDBusInterface iface(QStringLiteral("org.kde.tellico"), QStringLiteral("/NewStuff"), QStringLiteral("org.kde.tellico.newstuff"));
  QDBusReply<bool> reply = iface.call(QStringLiteral("installScript"), scriptFile);
  QVERIFY(reply.isValid());
  QCOMPARE(reply.value(), true);
  QVERIFY(dir.exists(QStringLiteral("dark_horse_comics.py")));

  reply = iface.call(QStringLiteral("removeScript"), scriptFile);
  QVERIFY(reply.isValid());
  QCOMPARE(reply.value(), true);
  QVERIFY(!dir.exists()); // complete directory should not exist
}
