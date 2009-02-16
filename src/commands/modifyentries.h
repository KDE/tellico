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

#ifndef TELLICO_MODIFYENTRIES_H
#define TELLICO_MODIFYENTRIES_H

#include "../datavectors.h"

#include <QUndoCommand>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class ModifyEntries : public QUndoCommand {

public:
  ModifyEntries(Data::CollPtr coll, const Data::EntryList& oldEntries, const Data::EntryList& newEntries);
  ModifyEntries(QUndoCommand* parent, Data::CollPtr coll, const Data::EntryList& oldEntries, const Data::EntryList& newEntries);

  virtual void redo();
  virtual void undo();

private:
  void swapValues();

  Data::CollPtr m_coll;
  Data::EntryList m_oldEntries;
  Data::EntryList m_entries;
  bool m_needToSwap : 1;
};

  } // end namespace
}

#endif
