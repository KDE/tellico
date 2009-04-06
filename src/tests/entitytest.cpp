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
#include "entitytest.h"
#include "entitytest.moc"
#include "../tellico_utils.h"

QTEST_KDEMAIN_CORE( EntityTest )

#define QL1(x) QString::fromLatin1(x)

void EntityTest::testEntities() {
  QFETCH(QByteArray, data);
  QFETCH(QString, expectedString);

  QCOMPARE(Tellico::decodeHTML(data), expectedString);
}

void EntityTest::testEntities_data() {
  QTest::addColumn<QByteArray>("data");
  QTest::addColumn<QString>("expectedString");

  QTest::newRow("robby") << QByteArray("robby") << QL1("robby");
  QTest::newRow("&fake;") << QByteArray("&fake;") << QL1("&fake;");
  QTest::newRow("&#48;") << QByteArray("&#48;") << QL1("0");
  QTest::newRow("robby&#48;robb") << QByteArray("robby&#48;robby") << QL1("robby0robby");
}
