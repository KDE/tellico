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

#include "reorderfields.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::ReorderFields;

ReorderFields::ReorderFields(Data::CollPtr coll_, const Data::FieldVec& oldFields_,
                             const Data::FieldVec& newFields_)
    : KCommand()
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

void ReorderFields::execute() {
  if(!m_coll) {
    return;
  }
  m_coll->reorderFields(m_newFields);
  Controller::self()->reorderedFields(m_coll);
}

void ReorderFields::unexecute() {
  if(!m_coll) {
    return;
  }
  m_coll->reorderFields(m_oldFields);
  Controller::self()->reorderedFields(m_coll);
}

QString ReorderFields::name() const {
  return i18n("Reorder Fields");
}
