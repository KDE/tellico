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

//TODO: fix this to be able to run

#include "qtest_kde.h"
#include "lcctest.h"
#include "lcctest.moc"
//#include "../listviewcomparison.h"

QTEST_KDEMAIN_CORE( LccTest )

void LccTest::testSorting() {
//  Tellico::Data::FieldPtr f;
//  Tellico::LCCComparison comp(f);

  QString lcc1 = QLatin1String("DA870.F64");
  QString lcc2 = QLatin1String("DK602.3.B76 1996");
  QString lcc3 = QLatin1String("Q335.P416 1994");

//  QCOMPARE(comp.compare(lcc1, lcc2), -1);
}
