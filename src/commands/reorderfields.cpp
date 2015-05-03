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

#include "reorderfields.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

using Tellico::Command::ReorderFields;

ReorderFields::ReorderFields(Tellico::Data::CollPtr coll_, const Tellico::Data::FieldList& oldFields_,
                             const Tellico::Data::FieldList& newFields_)
    : QUndoCommand(i18n("Reorder Fields"))
    , m_coll(coll_)
    , m_oldFields(oldFields_)
    , m_newFields(newFields_)
{
  if(!m_coll) {
    myDebug() << "null collection pointer";
  } else if(m_oldFields.count() != m_newFields.count()) {
    myDebug() << "unequal number of fields";
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
