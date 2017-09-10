/***************************************************************************
    Copyright (C) 2010 Robby Stephenson <robby@periapsis.org>
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

#include "comparisontest.h"
#include "../models/stringcomparison.h"

#include <QTest>

QTEST_APPLESS_MAIN( ComparisonTest )

void ComparisonTest::testNumber() {
  QFETCH(QString, string1);
  QFETCH(QString, string2);
  QFETCH(int, res);

  Tellico::NumberComparison comp;

  QCOMPARE(comp.compare(string1, string2), res);
}

void ComparisonTest::testNumber_data() {
  QTest::addColumn<QString>("string1");
  QTest::addColumn<QString>("string2");
  QTest::addColumn<int>("res");

  QTest::newRow("null") << QString() << QString() << 0;
  QTest::newRow("empty") << QString("") << QString("") << 0;
  QTest::newRow("< 0") << QString("") << QString("0") << -1;
  QTest::newRow("> 0") << QString("0") << QString("") << 1;
  QTest::newRow("=1 1") << QString("1") << QString("1") << 0;
  QTest::newRow("< 1") << QString("0") << QString("1") << -1;
  QTest::newRow("> 1") << QString("1") << QString("0") << 1;
  QTest::newRow("> -1") << QString("0") << QString("-1") << 1;
  QTest::newRow("< -1") << QString("-1") << QString("0") << -1;
  QTest::newRow("> 10") << QString("10") << QString("5") << 5;
  QTest::newRow("< 10") << QString("5") << QString("10") << -5;
  QTest::newRow("multiple1") << QString("1; 2") << QString("2") << -1;
  QTest::newRow("multiple2") << QString("3; 2") << QString("2") << 1;
  QTest::newRow("multiple3") << QString("2") << QString("2; 3") << -1;
  QTest::newRow("multiple4") << QString("1; 2") << QString("1; 3") << -1;
  QTest::newRow("float1") << QString("5.1") << QString("6.9") << -2;
  QTest::newRow("float2") << QString("5.1") << QString("5.2") << -1;
  QTest::newRow("float3") << QString("5.2") << QString("5.1") << 1;
  QTest::newRow("float4") << QString("5.1") << QString("5.1") << 0;
}
