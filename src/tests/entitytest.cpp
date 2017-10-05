/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "entitytest.h"

#include "../utils/string_utils.h"

#include <QTest>

QTEST_APPLESS_MAIN( EntityTest )

#define QL1(x) QString::fromLatin1(x)
#define QU8(x) QString::fromUtf8(x)

void EntityTest::testEntities() {
  QFETCH(QByteArray, data);
  QFETCH(QString, expectedString);

  QCOMPARE(Tellico::decodeHTML(data), expectedString);
}

void EntityTest::testEntities_data() {
  QTest::addColumn<QByteArray>("data");
  QTest::addColumn<QString>("expectedString");

  QTest::newRow("robby") << QByteArray("robby") << QL1("robby");
  QTest::newRow("&fake;") << QByteArray("&fake;") << QL1("&fake;");
  QTest::newRow("&#48;") << QByteArray("&#48;") << QL1("0");
  QTest::newRow("robby&#48;robb") << QByteArray("robby&#48;robby") << QL1("robby0robby");
}

void EntityTest::testAccents() {
  QFETCH(QString, inputString);
  QFETCH(QString, expectedString);

  QCOMPARE(Tellico::removeAccents(inputString), expectedString);
}

void EntityTest::testAccents_data() {
  QTest::addColumn<QString>("inputString");
  QTest::addColumn<QString>("expectedString");

  QTest::newRow("robby") << QL1("robby") << QL1("robby");
  QTest::newRow("jose") << QU8("José Guzmán") << QL1("Jose Guzman");
  QTest::newRow("inarritu") << QU8("Alejandro González Iñárritu") << QL1("Alejandro Gonzalez Inarritu");
  QTest::newRow("harakiri") << QU8("'Shitsurakuen': jôbafuku onna harakiri") << QL1("'Shitsurakuen': jobafuku onna harakiri");
  QTest::newRow("svet") << QU8("Tmavomodrý Svět") << QL1("Tmavomodry Svet");
  QTest::newRow("russian") << QU8("Возвращение Супермена") << QU8("Возвращение Супермена");
  QTest::newRow("chinese") << QU8("湖南科学技术出版社") << QU8("湖南科学技术出版社");
}
