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

#ifndef TELLICO_FILTERCOMMAND_H
#define TELLICO_FILTERCOMMAND_H

#include "../datavectors.h"

#include <QUndoCommand>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class FilterCommand : public QUndoCommand  {

public:
  enum Mode {
    FilterAdd,
    FilterModify,
    FilterRemove
  };

  FilterCommand(Mode mode, FilterPtr activeFilter, FilterPtr oldFilter=FilterPtr());

  virtual void redo();
  virtual void undo();

private:
  Mode m_mode;
  FilterPtr m_activeFilter;
  FilterPtr m_oldFilter;
};

  } // end namespace
}

#endif
