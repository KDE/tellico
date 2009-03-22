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
#include "lcctest.h"
#include "lcctest.moc"
#include "../utils/stringcomparison.h"

QTEST_KDEMAIN_CORE( LccTest )

// see http://library.dts.edu/Pages/RM/Helps/lc_call.shtml

void LccTest::testSorting() {
  Tellico::LCCComparison comp;

  QString lcc1 = QLatin1String("B17.A4 1990");
  QString lcc2 = QLatin1String("B17.G3 1964");
  QString lcc3 = QLatin1String("B17.6.B64 Z59");
  QString lcc4 = QLatin1String("B17.6.B8 G37");
  QString lcc5 = QLatin1String("BR123.H4");
  QString lcc6 = QLatin1String("BR123.H56");
  QString lcc7 = QLatin1String("BR123.H56 1935");
  QString lcc8 = QLatin1String("BR123.H56 B5");
  QString lcc9 = QLatin1String("BR123.H8 v.1");

  QCOMPARE(comp.compare(lcc1, lcc2) < 0, true);
  QCOMPARE(comp.compare(lcc2, lcc3) < 0, true);
  QCOMPARE(comp.compare(lcc3, lcc4) < 0, true);
  QCOMPARE(comp.compare(lcc1, lcc4) < 0, true);
  QCOMPARE(comp.compare(lcc4, lcc5) < 0, true);
  QCOMPARE(comp.compare(lcc5, lcc6) < 0, true);
  QCOMPARE(comp.compare(lcc6, lcc7) < 0, true);
  QCOMPARE(comp.compare(lcc6, lcc8) < 0, true);
  QCOMPARE(comp.compare(lcc7, lcc8) < 0, true);
  QCOMPARE(comp.compare(lcc8, lcc9) < 0, true);
}
