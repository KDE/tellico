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

#include "renamecollection.h"
#include "../document.h"
#include "../collection.h"

#include <klocale.h>

using Tellico::Command::RenameCollection;

RenameCollection::RenameCollection(Data::CollPtr coll_, const QString& newTitle_)
    : KCommand()
    , m_coll(coll_)
    , m_newTitle(newTitle_)
{
}

void RenameCollection::execute() {
  if(!m_coll) {
    return;
  }
  m_oldTitle = m_coll->title();
  Data::Document::self()->renameCollection(m_newTitle);
}

void RenameCollection::unexecute() {
  if(!m_coll) {
    return;
  }
  Data::Document::self()->renameCollection(m_oldTitle);
}

QString RenameCollection::name() const {
  return i18n("Rename Collection");
}
