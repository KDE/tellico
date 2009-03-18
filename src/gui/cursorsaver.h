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

#ifndef TELLICO_GUI_CURSORSAVER_H
#define TELLICO_GUI_CURSORSAVER_H

#include <QCursor>

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class CursorSaver {
public:
  CursorSaver(const QCursor& cursor = QCursor(Qt::WaitCursor));
  ~CursorSaver();

  void restore();

private:
  bool m_restored : 1;
};

  } // end namespace
} // end namespace
#endif
