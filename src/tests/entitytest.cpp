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
#include "entitytest.h"
#include "entitytest.moc"
#include "../tellico_utils.h"

QTEST_KDEMAIN_CORE( EntityTest )

void EntityTest::testEntities() {
  QCOMPARE(Tellico::decodeHTML("robby"), QLatin1String("robby"));
  QCOMPARE(Tellico::decodeHTML("&fake;"), QLatin1String("&fake;"));
  QCOMPARE(Tellico::decodeHTML("&#48;"), QLatin1String("0"));
  QCOMPARE(Tellico::decodeHTML("robby&#48;robby"), QLatin1String("robby0robby"));
}
