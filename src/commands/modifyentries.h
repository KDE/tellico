/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
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

#include <kcommand.h>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class ModifyEntries : public KCommand {

public:
  ModifyEntries(Data::CollPtr coll, const Data::EntryVec& oldEntries, const Data::EntryVec& newEntries);

  virtual void execute();
  virtual void unexecute();
  virtual QString name() const;

private:
  void swapValues();

  Data::CollPtr m_coll;
  Data::EntryVec m_oldEntries;
  Data::EntryVec m_entries;
  bool m_needToSwap : 1;
};

  } // end namespace
}

#endif
