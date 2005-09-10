/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_ADDENTRIES_H
#define TELLICO_ADDENTRIES_H

#include "../datavectors.h"

#include <kcommand.h>

#include <qguardedptr.h>

namespace Tellico {
  namespace Data {
    class Collection;
  }
  namespace Command {

/**
 * @author Robby Stephenson
 */
class AddEntries : public KCommand  {

public:
  AddEntries(Data::Collection* coll, Data::EntryVec entries);

  virtual void execute();
  virtual void unexecute();
  virtual QString name() const;

private:
  QGuardedPtr<Data::Collection> m_coll;
  Data::EntryVec m_entries;
};

  } // end namespace
}

#endif
