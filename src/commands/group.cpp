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

#include "group.h"

using Tellico::Command::Group;

QString Group::name() const {
  if(m_commands.count() == 1) {
    return m_commands.getFirst()->name();
  }
  return KMacroCommand::name();
}
