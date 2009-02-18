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

#ifndef FORMATTEST_H
#define FORMATTEST_H

#include <QObject>

class FormatTest : public QObject {
Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void testCapitalization();
  void testCapitalization_data();
  void testTitle();
  void testTitle_data();
  void testName();
  void testName_data();
};

#endif
