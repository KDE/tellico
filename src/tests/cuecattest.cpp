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

#include "cuecattest.h"

#include "../utils/upcvalidator.h"

#include <QTest>

QTEST_APPLESS_MAIN( CueCatTest )

Q_DECLARE_METATYPE(QValidator::State)

#define QL1(x) QString::fromLatin1(x)

void CueCatTest::initTestCase() {
  qRegisterMetaType<QValidator::State>();
}

void CueCatTest::testDecode() {
  QFETCH(QString, string);
  QFETCH(QString, expectedString);
  QFETCH(QValidator::State, state);

  QString originalString = string;
  QCOMPARE(Tellico::CueCat::decode(string), state);
  QCOMPARE(string, expectedString);
  Tellico::UPCValidator v(this);
  int pos = 0;
  QCOMPARE(v.validate(originalString, pos), state);
}

void CueCatTest::testDecode_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("expectedString");
  QTest::addColumn<QValidator::State>("state");

  QTest::newRow("My name is robby") << QL1("My name is robby")
                                    << QL1("My name is robby")
                                    << QValidator::Invalid;
  QTest::newRow(".C3nZC3nZC3nYCxP2Dxb1CNnY") << QL1(".C3nZC3nZC3nYCxP2Dxb1CNnY")
                                             << QL1(".C3nZC3nZC3nYCxP2Dxb1CNnY")
                                             << QValidator::Intermediate;
  QTest::newRow(".C3nZC3nZC3nYCxP2Dxb1CNnY.cGen.ENr7C3fZCNT7ENz3Ca.")
         << QL1(".C3nZC3nZC3nYCxP2Dxb1CNnY.cGen.ENr7C3fZCNT7ENz3Ca.")
         << QL1("9780201889543")
         << QValidator::Acceptable;
}

void CueCatTest::testUpcValidate() {
  QFETCH(QString, string);
  QFETCH(QString, expectedString);
  QFETCH(QValidator::State, state);
  QFETCH(bool, isbn);

  Tellico::UPCValidator v(this);
  int pos = 0;
  v.setCheckISBN(isbn);
  QCOMPARE(v.validate(string, pos), state);
  v.fixup(string);
  QCOMPARE(string, expectedString);
}

void CueCatTest::testUpcValidate_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("expectedString");
  QTest::addColumn<QValidator::State>("state");
  QTest::addColumn<bool>("isbn");

  // TODO: return QValidator::Acceptable if check sum is correct
  QTest::newRow("Acceptable") << QL1("796030114977") << QL1("796030114977") << QValidator::Intermediate << false;
  QTest::newRow("Intermediate") << QL1("79603011497") << QL1("79603011497") << QValidator::Intermediate << false;
  QTest::newRow("Intermediate space") << QL1("79603011497 ") << QL1("79603011497") << QValidator::Invalid << false;
  QTest::newRow("Intermediate space number") << QL1("79603011497 7") << QL1("79603011497") << QValidator::Invalid << false;
  QTest::newRow("ISBN invalid") << QL1("9780940016750") << QL1("9780940016750") << QValidator::Intermediate << false;
  QTest::newRow("ISBN1") << QL1("9780940016750") << QL1("978-0-940016-75-0") << QValidator::Acceptable << true;
  QTest::newRow("ISBN2") << QL1("978-0940016750") << QL1("978-0-940016-75-0") << QValidator::Acceptable << true;
}
