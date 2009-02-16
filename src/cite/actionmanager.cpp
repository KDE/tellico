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

#include "actionmanager.h"
#include "lyxpipe.h"
#include "clipboard.h"
#include "openoffice.h"
#include "../entry.h"
#include "../tellico_debug.h"

using Tellico::Cite::ActionManager;

Tellico::Cite::ActionManager* ActionManager::self() {
  static ActionManager self;
  return &self;
}

ActionManager::ActionManager() : m_action(0) {
}

ActionManager::~ActionManager() {
  delete m_action;
}

bool ActionManager::connect(Tellico::Cite::CiteAction action_) {
  if(m_action && m_action->type() == action_) {
    return m_action->connect();
  } else if(m_action) {
    delete m_action;
    m_action = 0;
  }

  switch(action_) {
    case Cite::CiteClipboard:
       m_action = new Clipboard();
       break;

    case Cite::CiteLyxpipe:
      m_action = new Lyxpipe();
      break;

    case Cite::CiteOpenOffice:
      m_action = new OpenOffice();
      break;
  }
  return m_action ? m_action->connect() : false;
}

bool ActionManager::cite(Tellico::Cite::CiteAction action_, Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    myDebug() << "ActionManager::cite() - no entries to cite" << endl;
    return false;
  }
  if(m_action && m_action->type() != action_) {
    delete m_action;
    m_action = 0;
  }
  if(!m_action && !connect(action_)) {
    myDebug() << "ActionManager::cite() - unable to connect" << endl;
    return false;
  }
  if(!m_action) {
    myDebug() << "ActionManager::cite() - no action found" << endl;
    return false;
  }

  return m_action->cite(entries_);
}

bool ActionManager::isEnabled(Tellico::Cite::CiteAction action_) {
  if(action_ == CiteOpenOffice) {
    return OpenOffice::hasLibrary();
  }
  return true;
}
