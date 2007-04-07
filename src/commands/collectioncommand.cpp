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

#include "collectioncommand.h"
#include "../document.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::CollectionCommand;

CollectionCommand::CollectionCommand(Mode mode_, Data::CollPtr origColl_, Data::CollPtr newColl_)
    : KCommand()
    , m_mode(mode_)
    , m_origColl(origColl_)
    , m_newColl(newColl_)
    , m_cleanup(DoNothing)
{
#ifndef NDEBUG
// just some sanity checking
  if(m_origColl == 0 || m_newColl == 0) {
    myDebug() << "CollectionCommand() - null collection pointer" << endl;
  }
#endif
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

void CollectionCommand::execute() {
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

void CollectionCommand::unexecute() {
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

QString CollectionCommand::name() const {
  switch(m_mode) {
    case Append:
      return i18n("Append Collection");
    case Merge:
      return i18n("Merge Collection");
    case Replace:
      return i18n("Replace Collection");
  }
  // hush warnings
  return QString::null;
}

void CollectionCommand::copyFields() {
  m_origFields.clear();
  Data::FieldVec fieldsToCopy = m_origColl->fields();
  for(Data::FieldVec::Iterator field = fieldsToCopy.begin(); field != fieldsToCopy.end(); ++field) {
    m_origFields.append(new Data::Field(*field));
  }
}
