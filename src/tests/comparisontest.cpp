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
#include "../config/tellico_config.h"

#include <QTest>

QTEST_GUILESS_MAIN( ComparisonTest )

void ComparisonTest::initTestCase() {
  Tellico::Config::setArticlesString(QStringLiteral("the,l'"));
}

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
  QTest::newRow("< 0") << QString() << QStringLiteral("0") << -1;
  QTest::newRow("> 0") << QStringLiteral("0") << QString() << 1;
  QTest::newRow("=1 1") << QStringLiteral("1") << QStringLiteral("1") << 0;
  QTest::newRow("< 1") << QStringLiteral("0") << QStringLiteral("1") << -1;
  QTest::newRow("> 1") << QStringLiteral("1") << QStringLiteral("0") << 1;
  QTest::newRow("> -1") << QStringLiteral("0") << QStringLiteral("-1") << 1;
  QTest::newRow("< -1") << QStringLiteral("-1") << QStringLiteral("0") << -1;
  QTest::newRow("> 10") << QStringLiteral("10") << QStringLiteral("5") << 5;
  QTest::newRow("< 10") << QStringLiteral("5") << QStringLiteral("10") << -5;
  QTest::newRow("multiple1") << QStringLiteral("1; 2") << QStringLiteral("2") << -1;
  QTest::newRow("multiple2") << QStringLiteral("3; 2") << QStringLiteral("2") << 1;
  QTest::newRow("multiple3") << QStringLiteral("2") << QStringLiteral("2; 3") << -1;
  QTest::newRow("multiple4") << QStringLiteral("1; 2") << QStringLiteral("1; 3") << -1;
  QTest::newRow("float1") << QStringLiteral("5.1") << QStringLiteral("6.9") << -2;
  QTest::newRow("float2") << QStringLiteral("5.1") << QStringLiteral("5.2") << -1;
  QTest::newRow("float3") << QStringLiteral("5.2") << QStringLiteral("5.1") << 1;
  QTest::newRow("float4") << QStringLiteral("5.1") << QStringLiteral("5.1") << 0;
}

void ComparisonTest::testLCC() {
  QFETCH(QString, string1);
  QFETCH(QString, string2);
  QFETCH(int, res);

  Tellico::LCCComparison comp;

  QCOMPARE(comp.compare(string1, string2), res);
}

void ComparisonTest::testLCC_data() {
  QTest::addColumn<QString>("string1");
  QTest::addColumn<QString>("string2");
  QTest::addColumn<int>("res");

  QTest::newRow("null") << QString() << QString() << 0;
  QTest::newRow("null1") << QString() << QStringLiteral("B") << -1;
  QTest::newRow("null2") << QStringLiteral("B") << QString() << 1;
  QTest::newRow("test1") << QStringLiteral("BX932 .C53 1993") << QStringLiteral("BX2230.3") << -1;
  QTest::newRow("test2") << QStringLiteral("BX932 .C53 1993") << QStringLiteral("BX2380 .R67 2002") << -1;
  QTest::newRow("test3") << QStringLiteral("AE25 E3 2002") << QStringLiteral("AE5 E333 2003") << 1;
}

void ComparisonTest::testDate() {
  QFETCH(QString, string1);
  QFETCH(QString, string2);
  QFETCH(int, res);

  Tellico::ISODateComparison comp;

  QCOMPARE(comp.compare(string1, string2), res);
}

void ComparisonTest::testDate_data() {
  QTest::addColumn<QString>("string1");
  QTest::addColumn<QString>("string2");
  QTest::addColumn<int>("res");

  QTest::newRow("null") << QString() << QString() << 0;
  QTest::newRow("null1") << QString() << QStringLiteral("2001") << -1;
  QTest::newRow("null2") << QStringLiteral("2001") << QString() << 1;
  QTest::newRow("test1") << QStringLiteral("2001-9-8") << QStringLiteral("2001-10-12") << -1;
  QTest::newRow("test2") << QStringLiteral("1998") << QStringLiteral("2020-01-01") << -1;
  QTest::newRow("test3") << QStringLiteral("2008--") << QStringLiteral("2008-1-1") << 0;
  QTest::newRow("test5") << QStringLiteral("2008-2-2") << QStringLiteral("2008-02-02") << 0;
}

void ComparisonTest::testTitle() {
  QFETCH(QString, string1);
  QFETCH(QString, string2);
  QFETCH(int, res);

  Tellico::TitleComparison comp;

  QCOMPARE(comp.compare(string1, string2), res);
}

void ComparisonTest::testTitle_data() {
  QTest::addColumn<QString>("string1");
  QTest::addColumn<QString>("string2");
  QTest::addColumn<int>("res");

  QTest::newRow("null") << QString() << QString() << 0;
  QTest::newRow("null1") << QString() << QStringLiteral("The One") << -1;
  QTest::newRow("null2") << QStringLiteral("The One") << QString() << 1;
  // not that they're equal, but that "The One" should be sorted under "One"
  QTest::newRow("test3") << QStringLiteral("The One") << QStringLiteral("One, The") << -1;
  QTest::newRow("test4") << QStringLiteral("The One") << QStringLiteral("one, the") << -1;
  QTest::newRow("test5") << QStringLiteral("l'One") << QStringLiteral("one, the") << -1;
  QTest::newRow("test6") << QStringLiteral("l'One") << QStringLiteral("the one") << 0;
  QTest::newRow("test7") << QStringLiteral("All One") << QStringLiteral("the one") << -1;
}
