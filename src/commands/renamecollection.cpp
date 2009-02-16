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
