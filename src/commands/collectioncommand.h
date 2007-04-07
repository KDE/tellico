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

#ifndef TELLICO_COLLECTIONCOMMAND_H
#define TELLICO_COLLECTIONCOMMAND_H

#include "../collection.h"
#include "../datavectors.h"

#include <kcommand.h>
#include <ksharedptr.h>
#include <kurl.h>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class CollectionCommand : public KCommand {
public:
  enum Mode {
    Append,
    Merge,
    Replace
  };

  CollectionCommand(Mode mode, Data::CollPtr currentColl, Data::CollPtr newColl);
  ~CollectionCommand();

  virtual void execute();
  virtual void unexecute();
  virtual QString name() const;

private:
  void copyFields();

  Mode m_mode;
  Data::CollPtr m_origColl;
  Data::CollPtr m_newColl;

  KURL m_origURL;
  Data::FieldVec m_origFields;
  Data::MergePair m_mergePair;
  // for the Replace case, the collection that got replaced needs to be cleared
  enum CleanupMode {
    DoNothing, ClearOriginal, ClearNew
  };
  CleanupMode m_cleanup;
};

  } // end namespace
}

#endif
