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

#ifndef CSVTEST_H
#define CSVTEST_H

#include <QObject>

namespace Tellico {
  class CSVParser;
}

class CsvTest : public QObject {
Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void cleanupTestCase();

  void init();
  void testAll();
  void testAll_data();

private:
  Tellico::CSVParser* p;
};

#endif
