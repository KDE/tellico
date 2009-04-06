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

#ifndef CUECATTEST_H
#define CUECATTEST_H

#include <QObject>

class CueCatTest : public QObject {
Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void testDecode();
  void testDecode_data();
};

#endif
