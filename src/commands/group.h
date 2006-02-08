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

#ifndef TELLICO_COMMAND_GROUP_H
#define TELLICO_COMMAND_GROUP_H

#include <kcommand.h>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class Group : public KMacroCommand {

public:
  Group(const QString& name) : KMacroCommand(name) {}

  virtual QString name() const;

  bool isEmpty() const { return m_commands.count() == 0; }
};

  } // end namespace
}

#endif
