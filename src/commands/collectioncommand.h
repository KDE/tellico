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

#ifndef TELLICO_COLLECTIONCOMMAND_H
#define TELLICO_COLLECTIONCOMMAND_H

#include "../datavectors.h"

#include <kurl.h>

#include <QUndoCommand>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class CollectionCommand : public QUndoCommand {
public:
  enum Mode {
    Append,
    Merge,
    Replace
  };

  CollectionCommand(Mode mode, Data::CollPtr currentColl, Data::CollPtr newColl);
  ~CollectionCommand();

  virtual void redo();
  virtual void undo();

private:
  void copyFields();

  Mode m_mode;
  Data::CollPtr m_origColl;
  Data::CollPtr m_newColl;

  KUrl m_origURL;
  Data::FieldList m_origFields;
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
