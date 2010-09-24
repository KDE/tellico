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

#include "lccntest.h"
#include "lccntest.moc"
#include "qtest_kde.h"

#include "../utils/lccnvalidator.h"

// see http://www.loc.gov/marc/lccn_structure.html
// see http://www.loc.gov/marc/lccn-namespace.html
// see http://catalog.loc.gov/help/number.htm

QTEST_KDEMAIN_CORE( LccnTest )

Q_DECLARE_METATYPE(QValidator::State)

#define QL1(x) QString::fromLatin1(x)

void LccnTest::initTestCase() {
  qRegisterMetaType<QValidator::State>();
}

void LccnTest::testValidation() {
  QFETCH(QString, string);
  QFETCH(QValidator::State, state);

  Tellico::LCCNValidator val(0);
  int pos = 0;

  QCOMPARE(val.validate(string, pos), state);
}

void LccnTest::testValidation_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QValidator::State>("state");

  QTest::newRow("89-456") << QL1("89-456") << QValidator::Acceptable;
  QTest::newRow("2001-1114") << QL1("2001-1114") << QValidator::Acceptable;
  QTest::newRow("gm 71-2450") << QL1("gm 71-2450") << QValidator::Acceptable;
}

void LccnTest::testFormalization() {
  QFETCH(QString, string);
  QFETCH(QString, result);

  QCOMPARE(Tellico::LCCNValidator::formalize(string), result);
}

void LccnTest::testFormalization_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("result");

  QTest::newRow("89-456") << QL1("89-456") << QL1("89000456");
  QTest::newRow("2001-1114") << QL1("2001-1114") << QL1("2001001114");
  QTest::newRow("gm 71-2450 ") << QL1("gm 71-2450 ") << QL1("gm71002450");
  QTest::newRow("n78-890351 ") << QL1("n78-890351 ") << QL1("n78890351");
  QTest::newRow("n78-89035") << QL1("n78-89035") << QL1("n78089035");
  QTest::newRow("n 78890351") << QL1("n 78890351") << QL1("n78890351");
  QTest::newRow("85-2") << QL1("85-2") << QL1("85000002");
  QTest::newRow("75-425165//r75") << QL1("75-425165//r75") << QL1("75425165");
  QTest::newRow(" 79139101 /AC/r932") << QL1(" 79139101 /AC/r932") << QL1("79139101");
}
