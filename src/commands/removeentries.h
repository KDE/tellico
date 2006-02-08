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

#ifndef TELLICO_REMOVEENTRIES_H
#define TELLICO_REMOVEENTRIES_H

#include "../datavectors.h"

#include <kcommand.h>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class RemoveEntries : public KCommand  {

public:
  RemoveEntries(Data::CollPtr coll, const Data::EntryVec& entries);

  virtual void execute();
  virtual void unexecute();
  virtual QString name() const;

private:
  Data::CollPtr m_coll;
  Data::EntryVec m_entries;
};

  } // end namespace
}

#endif
