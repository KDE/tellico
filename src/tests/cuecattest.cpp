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

#include "qtest_kde.h"
#include "cuecattest.h"
#include "cuecattest.moc"
#include "../upcvalidator.h"

QTEST_KDEMAIN_CORE( CueCatTest )

void CueCatTest::testDecode() {
  QString bad = QString::fromLatin1("My name is robby");
  QCOMPARE(Tellico::CueCat::decode(bad), QValidator::Invalid);
  QCOMPARE(bad, QString::fromLatin1("My name is robby"));

  QString almost = QString::fromLatin1(".C3nZC3nZC3nYCxP2Dxb1CNnY");
  QCOMPARE(Tellico::CueCat::decode(almost), QValidator::Intermediate);
  QCOMPARE(almost, QString::fromLatin1(".C3nZC3nZC3nYCxP2Dxb1CNnY"));

  QString valid = QString::fromLatin1(".C3nZC3nZC3nYCxP2Dxb1CNnY.cGen.ENr7C3fZCNT7ENz3Ca.");
  QCOMPARE(Tellico::CueCat::decode(valid), QValidator::Acceptable);
  QCOMPARE(valid, QString::fromLatin1("9780201889543"));
}
