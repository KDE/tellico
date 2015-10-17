/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>

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

#include "collectioncommand.h"
#include "../collection.h"
#include "../document.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::CollectionCommand;

CollectionCommand::CollectionCommand(Mode mode_, Tellico::Data::CollPtr origColl_, Tellico::Data::CollPtr newColl_)
    : QUndoCommand()
    , m_mode(mode_)
    , m_origColl(origColl_)
    , m_newColl(newColl_)
    , m_cleanup(DoNothing)
{
#ifndef NDEBUG
// just some sanity checking
  if(!m_origColl || !m_newColl) {
    myDebug() << "null collection pointer";
  }
#endif
  switch(m_mode) {
    case Append:
      setText(i18n("Append Collection"));
      break;
    case Merge:
      setText(i18n("Merge Collection"));
      break;
    case Replace:
      setText(i18n("Replace Collection"));
      break;
  }
}

CollectionCommand::~CollectionCommand() {
  switch(m_cleanup) {
    case ClearOriginal:
      m_origColl->clear();
      break;
    case ClearNew:
      m_newColl->clear();
      break;
    default:
      break;
  }
}

void CollectionCommand::redo() {
  if(!m_origColl || !m_newColl) {
    return;
  }

  switch(m_mode) {
    case Append:
      copyFields();
      Data::Document::self()->appendCollection(m_newColl);
      Controller::self()->slotCollectionModified(m_origColl);
      break;

    case Merge:
      copyFields();
      m_mergePair = Data::Document::self()->mergeCollection(m_newColl);
      Controller::self()->slotCollectionModified(m_origColl);
      break;

    case Replace:
      // replaceCollection() makes the URL = "Unknown"
      m_origURL = Data::Document::self()->URL();
      Data::Document::self()->replaceCollection(m_newColl);
      Controller::self()->slotCollectionDeleted(m_origColl);
      Controller::self()->slotCollectionAdded(m_newColl);
      m_cleanup = ClearOriginal;
      break;
  }
}

void CollectionCommand::undo() {
  if(!m_origColl || !m_newColl) {
    return;
  }

  switch(m_mode) {
    case Append:
      Data::Document::self()->unAppendCollection(m_newColl, m_origFields);
      Controller::self()->slotCollectionModified(m_origColl);
      break;

    case Merge:
      Data::Document::self()->unMergeCollection(m_newColl, m_origFields, m_mergePair);
      Controller::self()->slotCollectionModified(m_origColl);
      break;

    case Replace:
      Data::Document::self()->replaceCollection(m_origColl);
      Data::Document::self()->setURL(m_origURL);
      Controller::self()->slotCollectionDeleted(m_newColl);
      Controller::self()->slotCollectionAdded(m_origColl);
      m_cleanup = ClearNew;
      break;
  }
}

void CollectionCommand::copyFields() {
  m_origFields.clear();
  foreach(Data::FieldPtr field, m_origColl->fields()) {
    m_origFields.append(Data::FieldPtr(new Data::Field(*field)));
  }
}
