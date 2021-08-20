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
#include "../collections/bibtexcollection.h"
#include "../document.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <algorithm>

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
    myDebug() << "CommandTest: null collection pointer";
  }
  if(m_origColl != Data::Document::self()->collection()) {
    myWarning() << "CollectionCommand: original collection is different than the current document";
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
      copyMacros();
      {
        auto existingEntries = m_origColl->entryIdList();
        Data::Document::self()->appendCollection(m_newColl);
        auto allEntries = m_origColl->entryIdList();

        // keep track of which entries were added by the append operation
        // by taking difference of the entry id lists
        m_addedEntries.clear();
        std::sort(existingEntries.begin(), existingEntries.end());
        std::sort(allEntries.begin(), allEntries.end());
        std::set_difference(allEntries.begin(), allEntries.end(),
                            existingEntries.begin(), existingEntries.end(),
                            std::back_inserter(m_addedEntries));
      }
      Controller::self()->slotCollectionModified(m_origColl);
      break;

    case Merge:
      copyFields();
      copyMacros();
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
      unCopyMacros();
      Data::Document::self()->unAppendCollection(m_origFields, m_addedEntries);
      Controller::self()->slotCollectionModified(m_origColl);
      break;

    case Merge:
      unCopyMacros();
      Data::Document::self()->unMergeCollection(m_origFields, m_mergePair);
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

void CollectionCommand::copyMacros() {
  // only applies to bibliographies
  if(m_origColl->type() != Data::Collection::Bibtex ||
     m_newColl->type() != Data::Collection::Bibtex) {
    return;
  }
  m_addedMacros.clear();
  // iterate over all macros in the new collection, check if they exist in the orig, add them if not
  // do not over write them
  // TODO: what to do if they clash?
  auto origColl = static_cast<Data::BibtexCollection*>(m_origColl.data());
  const QMap<QString, QString> origMacros = origColl->macroList();
  const QMap<QString, QString> newMacros = static_cast<Data::BibtexCollection*>(m_newColl.data())->macroList();

  auto i = newMacros.constBegin();
  while(i != newMacros.constEnd()) {
    if(!origMacros.contains(i.key())) {
      origColl->addMacro(i.key(), i.value());
      m_addedMacros.insert(i.key(), i.value());
    }
    ++i;
  }

  m_origPreamble = origColl->preamble();
  if(m_origPreamble.isEmpty()) {
    origColl->setPreamble(static_cast<Data::BibtexCollection*>(m_newColl.data())->preamble());
  }
}

void CollectionCommand::unCopyMacros() {
  // only applies to bibliographies
  if(m_origColl->type() != Data::Collection::Bibtex) {
    return;
  }
  auto origColl = static_cast<Data::BibtexCollection*>(m_origColl.data());
  // remove the macros added by the append/merge
  auto i = m_addedMacros.constBegin();
  while(i != m_addedMacros.constEnd()) {
    origColl->removeMacro(i.key());
    ++i;
  }
  origColl->setPreamble(m_origPreamble);
}
