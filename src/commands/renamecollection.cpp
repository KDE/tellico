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

#include "renamecollection.h"
#include "../document.h"
#include "../collection.h"

#include <klocale.h>

using Tellico::Command::RenameCollection;

RenameCollection::RenameCollection(Tellico::Data::CollPtr coll_, const QString& newTitle_)
    : QUndoCommand(i18n("Rename Collection"))
    , m_coll(coll_)
    , m_newTitle(newTitle_)
{
}

void RenameCollection::redo() {
  if(!m_coll) {
    return;
  }
  m_oldTitle = m_coll->title();
  Data::Document::self()->renameCollection(m_newTitle);
}

void RenameCollection::undo() {
  if(!m_coll) {
    return;
  }
  Data::Document::self()->renameCollection(m_oldTitle);
}
