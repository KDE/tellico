/***************************************************************************
    copyright            : (C) 2007-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_XMPHANDLER_H
#define TELLICO_XMPHANDLER_H

class QString;

namespace Tellico {

class XMPHandler {
public:
  XMPHandler();
  ~XMPHandler();

  QString extractXMP(const QString& file);

  static bool isXMPEnabled();

private:
  void init();

  static int s_initCount;
};

}
#endif
