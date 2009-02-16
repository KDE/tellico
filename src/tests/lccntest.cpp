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
#include "lccntest.h"
#include "lccntest.moc"
#include "../lccnvalidator.h"

// see http://www.loc.gov/marc/lccn_structure.html
// see http://catalog.loc.gov/help/number.htm

QTEST_KDEMAIN_CORE( LccnTest )

void LccnTest::testValidation() {
  QString lccn1 = QString::fromLatin1("89-456");
  QString lccn2 = QString::fromLatin1("2001-1114");
  QString lccn3 = QString::fromLatin1("gm 71-2450");

  Tellico::LCCNValidator val(0);
  int pos = 0;

  QCOMPARE(val.validate(lccn1, pos), QValidator::Acceptable);
  QCOMPARE(val.validate(lccn2, pos), QValidator::Acceptable);
  QCOMPARE(val.validate(lccn3, pos), QValidator::Acceptable);
}

void LccnTest::testFormalization() {
  QCOMPARE(Tellico::LCCNValidator::formalize(QString::fromLatin1("89-456")), QString::fromLatin1("89000456"));
  QCOMPARE(Tellico::LCCNValidator::formalize(QString::fromLatin1("2001-1114")), QString::fromLatin1("2001001114"));
  QCOMPARE(Tellico::LCCNValidator::formalize(QString::fromLatin1("gm 71-2450")), QString::fromLatin1("gm71002450"));
}
