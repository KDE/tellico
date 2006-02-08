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

#include "fieldcommand.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::FieldCommand;

FieldCommand::FieldCommand(Mode mode_, Data::CollPtr coll_,
                           Data::FieldPtr activeField_, Data::FieldPtr oldField_/*=0*/)
    : KCommand()
    , m_mode(mode_)
    , m_coll(coll_)
    , m_activeField(activeField_)
    , m_oldField(oldField_)
{
  if(!m_coll) {
    myDebug() << "FieldCommand() - null collection pointer" << endl;
  } else if(!m_activeField) {
    myDebug() << "FieldCommand() - null active field pointer" << endl;
  }
#ifndef NDEBUG
// just some sanity checking
  if(m_mode == FieldAdd && m_oldField != 0) {
    myDebug() << "FieldCommand() - adding field, but pointers are wrong" << endl;
  } else if(m_mode == FieldModify && m_oldField == 0) {
    myDebug() << "FieldCommand() - modifying field, but pointers are wrong" << endl;
  } else if(m_mode == FieldRemove && m_oldField != 0) {
    myDebug() << "FieldCommand() - removing field, but pointers are wrong" << endl;
  }
#endif
}

void FieldCommand::execute() {
  if(!m_coll || !m_activeField) {
    return;
  }

  switch(m_mode) {
    case FieldAdd:
      // if there's currently a field in the collection with the same name, it will get overwritten
      // so save a pointer to it here, the collection should not delete it
      m_oldField = m_coll->fieldByName(m_activeField->name());
      m_coll->addField(m_activeField);
      Controller::self()->addedField(m_coll, m_activeField);
      break;

    case FieldModify:
      m_coll->modifyField(m_activeField);
      Controller::self()->modifiedField(m_coll, m_oldField, m_activeField);
      break;

    case FieldRemove:
      m_coll->removeField(m_activeField);
      Controller::self()->removedField(m_coll, m_activeField);
      break;
  }
}

void FieldCommand::unexecute() {
  if(!m_coll || !m_activeField) {
    return;
  }

  switch(m_mode) {
    case FieldAdd:
      m_coll->removeField(m_activeField);
      Controller::self()->removedField(m_coll, m_activeField);
      if(m_oldField) {
        m_coll->addField(m_oldField);
        Controller::self()->addedField(m_coll, m_oldField);
      }
      break;

    case FieldModify:
      m_coll->modifyField(m_oldField);
      Controller::self()->modifiedField(m_coll, m_activeField, m_oldField);
      break;

    case FieldRemove:
      m_coll->addField(m_activeField);
      Controller::self()->addedField(m_coll, m_activeField);
      break;
  }
}

QString FieldCommand::name() const {
  switch(m_mode) {
    case FieldAdd:
      return i18n("Add %1 Field").arg(m_activeField->title());
    case FieldModify:
      return i18n("Modify %1 Field").arg(m_activeField->title());
    case FieldRemove:
      return i18n("Delete %1 Field").arg(m_activeField->title());
  }
  // hush warnings
  return QString::null;
}
