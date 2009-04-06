/***************************************************************************
    copyright            : (C) 2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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
