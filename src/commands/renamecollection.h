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

#ifndef TELLICO_RENAMECOLLECTION_H
#define TELLICO_RENAMECOLLECTION_H

#include "../datavectors.h"

#include <kcommand.h>

namespace Tellico {
  namespace Command {

/**
@author Robby Stephenson
*/
class RenameCollection : public KCommand {
public:
  RenameCollection(Data::CollPtr coll, const QString& newTitle);

  virtual void execute();
  virtual void unexecute();
  virtual QString name() const;

private:
  Data::CollPtr m_coll;
  QString m_oldTitle;
  QString m_newTitle;
};

  } // end namespace
} // end namespace
#endif
