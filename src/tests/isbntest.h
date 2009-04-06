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

#ifndef ISBNTEST_H
#define ISBNTEST_H

#include <QObject>

class IsbnTest : public QObject {
Q_OBJECT

private Q_SLOTS:
  void testFixup();
  void testFixup_data();
  void testIsbn10();
  void testIsbn10_data();
  void testIsbn13();
  void testIsbn13_data();
  void testComparison();
  void testComparison_data();
  void testListDifference();
  void testListDifference_data();
};

#endif
