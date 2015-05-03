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

#include "actionmanager.h"
#include "lyxpipe.h"
#include "clipboard.h"
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

  }
  return m_action ? m_action->connect() : false;
}

bool ActionManager::cite(Tellico::Cite::CiteAction action_, Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    myDebug() << "no entries to cite";
    return false;
  }
  if(m_action && m_action->type() != action_) {
    delete m_action;
    m_action = 0;
  }
  if(!m_action && !connect(action_)) {
    myDebug() << "unable to connect";
    return false;
  }
  if(!m_action) {
    myDebug() << "no action found";
    return false;
  }

  return m_action->cite(entries_);
}

bool ActionManager::hasError() const {
  return m_action && m_action->hasError();
}

QString ActionManager::errorString() const {
  return m_action ? m_action->errorString() : QString();
}
