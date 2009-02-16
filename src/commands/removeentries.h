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

#ifndef TELLICO_REMOVEENTRIES_H
#define TELLICO_REMOVEENTRIES_H

#include "../datavectors.h"

#include <QUndoCommand>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class RemoveEntries : public QUndoCommand  {

public:
  RemoveEntries(Data::CollPtr coll, const Data::EntryList& entries);

  virtual void redo();
  virtual void undo();

private:
  Data::CollPtr m_coll;
  Data::EntryList m_entries;
};

  } // end namespace
}

#endif
