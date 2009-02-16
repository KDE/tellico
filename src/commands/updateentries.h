/***************************************************************************
    copyright            : (C) 2006-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_UPDATEENTRIES_H
#define TELLICO_UPDATEENTRIES_H

#include "../datavectors.h"

#include <QUndoCommand>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class UpdateEntries : public QUndoCommand {

public:
  UpdateEntries(Data::CollPtr coll, Data::EntryPtr oldEntry, Data::EntryPtr newEntry, bool overWrite);

  virtual void redo();

private:
  Data::CollPtr m_coll;
  Data::EntryPtr m_oldEntry;
  Data::EntryPtr m_newEntry;
  bool m_overWrite : 1;
};

  } // end namespace
}

#endif
