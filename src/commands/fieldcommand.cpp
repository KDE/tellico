/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "fieldcommand.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

using Tellico::Command::FieldCommand;

FieldCommand::FieldCommand(Mode mode_, Tellico::Data::CollPtr coll_,
                           Tellico::Data::FieldPtr activeField_, Tellico::Data::FieldPtr oldField_/*=0*/)
    : QUndoCommand()
    , m_mode(mode_)
    , m_coll(coll_)
    , m_activeField(activeField_)
    , m_oldField(oldField_)
{
  init();
}

FieldCommand::FieldCommand(QUndoCommand* parent, Mode mode_, Tellico::Data::CollPtr coll_,
                           Tellico::Data::FieldPtr activeField_, Tellico::Data::FieldPtr oldField_/*=0*/)
    : QUndoCommand(parent)
    , m_mode(mode_)
    , m_coll(coll_)
    , m_activeField(activeField_)
    , m_oldField(oldField_)
{
  init();
}

void FieldCommand::init() {
  switch(m_mode) {
    case FieldAdd:
      setText(i18n("Add %1 Field", m_activeField->title()));
    case FieldModify:
      setText(i18n("Modify %1 Field", m_activeField->title()));
    case FieldRemove:
      setText(i18n("Delete %1 Field", m_activeField->title()));
  }

  if(!m_coll) {
    myDebug() << "null collection pointer";
  } else if(!m_activeField) {
    myDebug() << "null active field pointer";
  }
#ifndef NDEBUG
// just some sanity checking
  if(m_mode == FieldAdd && m_oldField) {
    myDebug() << "adding field, but pointers are wrong";
  } else if(m_mode == FieldModify && !m_oldField) {
    myDebug() << "modifying field, but pointers are wrong";
  } else if(m_mode == FieldRemove && m_oldField) {
    myDebug() << "removing field, but pointers are wrong";
  }
#endif
}

void FieldCommand::redo() {
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

void FieldCommand::undo() {
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
