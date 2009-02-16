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

#ifndef TELLICO_REORDERFIELDS_H
#define TELLICO_REORDERFIELDS_H

#include "../datavectors.h"

#include <QUndoCommand>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class ReorderFields : public QUndoCommand {

public:
  ReorderFields(Data::CollPtr coll, const Data::FieldList& oldFields, const Data::FieldList& newFields);

  virtual void redo();
  virtual void undo();

private:
  Data::CollPtr m_coll;
  Data::FieldList m_oldFields;
  Data::FieldList m_newFields;
};

  } // end namespace
}

#endif
