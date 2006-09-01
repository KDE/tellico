/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_NEWSTUFF_NEWSCRIPT_H
#define TELLICO_NEWSTUFF_NEWSCRIPT_H

#include <knewstuff/knewstuffsecure.h>

namespace Tellico {
  namespace NewStuff {

class Manager;

class NewScript : public KNewStuffSecure {
Q_OBJECT

public:
  NewScript(Manager* manager, QWidget* parentWidget = 0);
  virtual ~NewScript() {}

private:
  virtual void installResource();

  Manager* m_manager;
};

  }
}
#endif
