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

#include "filtercommand.h"
#include "../document.h"
#include "../collection.h"
#include "../controller.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Command::FilterCommand;

FilterCommand::FilterCommand(Mode mode_, FilterPtr activeFilter_, FilterPtr oldFilter_/*=0*/)
    : KCommand()
    , m_mode(mode_)
    , m_activeFilter(activeFilter_)
    , m_oldFilter(oldFilter_)
{
  if(!m_activeFilter) {
    myDebug() << "FilterCommand() - null active filter pointer" << endl;
  }
#ifndef NDEBUG
// just some sanity checking
  if(m_mode == FilterAdd && m_oldFilter != 0) {
    myDebug() << "FilterCommand() - adding field, but pointers are wrong" << endl;
  } else if(m_mode == FilterModify && m_oldFilter == 0) {
    myDebug() << "FilterCommand() - modifying field, but pointers are wrong" << endl;
  } else if(m_mode == FilterRemove && m_oldFilter != 0) {
    myDebug() << "FilterCommand() - removing field, but pointers are wrong" << endl;
  }
#endif
}

void FilterCommand::execute() {
  if(!m_activeFilter) {
    return;
  }

  switch(m_mode) {
    case FilterAdd:
      Data::Document::self()->collection()->addFilter(m_activeFilter);
      Controller::self()->addedFilter(m_activeFilter);
      break;

    case FilterModify:
      Data::Document::self()->collection()->removeFilter(m_oldFilter);
      Controller::self()->removedFilter(m_oldFilter);
      Data::Document::self()->collection()->addFilter(m_activeFilter);
      Controller::self()->addedFilter(m_activeFilter);
      break;

    case FilterRemove:
      Data::Document::self()->collection()->removeFilter(m_activeFilter);
      Controller::self()->removedFilter(m_activeFilter);
      break;
  }
}

void FilterCommand::unexecute() {
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

QString FilterCommand::name() const {
  switch(m_mode) {
    case FilterAdd:
      return i18n("Add Filter");
    case FilterModify:
      return i18n("Modify Filter");
    case FilterRemove:
      return i18n("Delete Filter");
  }
  // hush warnings
  return QString::null;
}
