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

#include "qtest_kde.h"
#include "cuecattest.h"
#include "cuecattest.moc"
#include "../utils/upcvalidator.h"

QTEST_KDEMAIN_CORE( CueCatTest )

Q_DECLARE_METATYPE(QValidator::State)

#define QL1(x) QString::fromLatin1(x)

void CueCatTest::initTestCase() {
  qRegisterMetaType<QValidator::State>();
}

void CueCatTest::testDecode() {
  QFETCH(QString, string);
  QFETCH(QString, expectedString);
  QFETCH(QValidator::State, state);

  QCOMPARE(Tellico::CueCat::decode(string), state);
  QCOMPARE(string, expectedString);
}

void CueCatTest::testDecode_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("expectedString");
  QTest::addColumn<QValidator::State>("state");

  QTest::newRow("My name is robby") << QL1("My name is robby") << QL1("My name is robby") << QValidator::Invalid;

  QTest::newRow(".C3nZC3nZC3nYCxP2Dxb1CNnY") << QL1(".C3nZC3nZC3nYCxP2Dxb1CNnY") << QL1(".C3nZC3nZC3nYCxP2Dxb1CNnY") << QValidator::Intermediate;

  QTest::newRow(".C3nZC3nZC3nYCxP2Dxb1CNnY.cGen.ENr7C3fZCNT7ENz3Ca.") << QL1(".C3nZC3nZC3nYCxP2Dxb1CNnY.cGen.ENr7C3fZCNT7ENz3Ca.") << QL1("9780201889543") << QValidator::Acceptable;
}
