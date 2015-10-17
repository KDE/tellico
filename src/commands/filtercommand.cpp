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

#include "filtercommand.h"
#include "../document.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

using Tellico::Command::FilterCommand;

FilterCommand::FilterCommand(Mode mode_, Tellico::FilterPtr activeFilter_, Tellico::FilterPtr oldFilter_/*=0*/)
    : QUndoCommand()
    , m_mode(mode_)
    , m_activeFilter(activeFilter_)
    , m_oldFilter(oldFilter_)
{
  if(!m_activeFilter) {
    myDebug() << "null active filter pointer";
  }
#ifndef NDEBUG
// just some sanity checking
  if(m_mode == FilterAdd && m_oldFilter) {
    myDebug() << "adding field, but pointers are wrong";
  } else if(m_mode == FilterModify && !m_oldFilter) {
    myDebug() << "modifying field, but pointers are wrong";
  } else if(m_mode == FilterRemove && m_oldFilter) {
    myDebug() << "removing field, but pointers are wrong";
  }
#endif
  switch(m_mode) {
    case FilterAdd:
      setText(i18n("Add Filter"));
      break;
    case FilterModify:
      setText(i18n("Modify Filter"));
      break;
    case FilterRemove:
      setText(i18n("Delete Filter"));
      break;
  }
}

void FilterCommand::redo() {
  if(!m_activeFilter) {
    return;
  }

  switch(m_mode) {
    case FilterAdd:
      Data::Document::self()->collection()->addFilter(m_activeFilter);
      Controller::self()->addedFilter(m_activeFilter);
      break;

    case FilterModify:
      Data::Document::self()->collection()->addFilter(m_activeFilter);
      Controller::self()->addedFilter(m_activeFilter);
      Data::Document::self()->collection()->removeFilter(m_oldFilter);
      Controller::self()->removedFilter(m_oldFilter);
      break;

    case FilterRemove:
      Data::Document::self()->collection()->removeFilter(m_activeFilter);
      Controller::self()->removedFilter(m_activeFilter);
      break;
  }
}

void FilterCommand::undo() {
  if(!m_activeFilter) {
    return;
  }

  switch(m_mode) {
    case FilterAdd:
      Data::Document::self()->collection()->removeFilter(m_activeFilter);
      Controller::self()->removedFilter(m_activeFilter);
      break;

    case FilterModify:
      Data::Document::self()->collection()->removeFilter(m_activeFilter);
      Controller::self()->removedFilter(m_activeFilter);
      Data::Document::self()->collection()->addFilter(m_oldFilter);
      Controller::self()->addedFilter(m_oldFilter);
      break;

    case FilterRemove:
      Data::Document::self()->collection()->addFilter(m_activeFilter);
      Controller::self()->addedFilter(m_activeFilter);
      break;
  }
}
