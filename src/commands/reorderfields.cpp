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

#include "reorderfields.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::ReorderFields;

ReorderFields::ReorderFields(Tellico::Data::CollPtr coll_, const Tellico::Data::FieldList& oldFields_,
                             const Tellico::Data::FieldList& newFields_)
    : QUndoCommand(i18n("Reorder Fields"))
    , m_coll(coll_)
    , m_oldFields(oldFields_)
    , m_newFields(newFields_)
{
  if(!m_coll) {
    myDebug() << "ReorderFieldsCommand() - null collection pointer" << endl;
  } else if(m_oldFields.count() != m_newFields.count()) {
    myDebug() << "ReorderFieldsCommand() - unequal number of fields" << endl;
  }
}

void ReorderFields::redo() {
  if(!m_coll) {
    return;
  }
  m_coll->reorderFields(m_newFields);
  Controller::self()->reorderedFields(m_coll);
}

void ReorderFields::undo() {
  if(!m_coll) {
    return;
  }
  m_coll->reorderFields(m_oldFields);
  Controller::self()->reorderedFields(m_coll);
}
