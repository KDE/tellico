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

#include <QTest>
#include <QDBusConnection>
#include <QDBusMessage>

QTEST_MAIN( NewStuffTest )

void NewStuffTest::initTestCase() {
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
  auto reply = conn.call(msg);
  QVERIFY(reply.type() == QDBusMessage::ReplyMessage);
}

void NewStuffTest::testNewStuff() {
  auto list = Tellico::NewStuff::Manager::self()->userTemplates();
  QVERIFY(list.isEmpty());
}
