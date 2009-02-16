/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_CITE_ACTIONMANAGER_H
#define TELLICO_CITE_ACTIONMANAGER_H

#include "../datavectors.h"
#include "handler.h"

namespace Tellico {
  namespace Cite {

enum CiteAction {
  CiteClipboard,
  CiteLyxpipe,
  CiteOpenOffice
};

/**
 * @author Robby Stephenson
 */
class Action {
public:
  Action() {}
  virtual ~Action() {}

  virtual CiteAction type() const = 0;
  virtual bool connect() { return true; }
  virtual bool cite(Data::EntryList entries) = 0;
  virtual State state() const { return Success; }
};

/**
 * @author Robby Stephenson
 */
class ActionManager {
public:
  static ActionManager* self();
  ~ActionManager();

  bool cite(CiteAction action, Data::EntryList entries);
  static bool isEnabled(CiteAction action);

private:
  ActionManager();
  bool connect(CiteAction action);

  Action* m_action;
};

  }
}

#endif
