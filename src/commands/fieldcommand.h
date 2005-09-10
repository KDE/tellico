/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FIELDCOMMAND_H
#define TELLICO_FIELDCOMMAND_H

#include "../datavectors.h"

#include <kcommand.h>

#include <qguardedptr.h>

namespace Tellico {
  namespace Data {
    class Collection;
  }
  namespace Command {

/**
 * @author Robby Stephenson
 */
class FieldCommand : public KCommand  {

public:
  enum Mode {
    FieldAdd,
    FieldModify,
    FieldRemove
  };

  FieldCommand(Mode mode, Data::Collection* coll, Data::Field* activeField, Data::Field* oldField=0);

  virtual void execute();
  virtual void unexecute();
  virtual QString name() const;

private:
  Mode m_mode;
  QGuardedPtr<Data::Collection> m_coll;
  Data::FieldPtr m_activeField;
  Data::FieldPtr m_oldField;
};

  } // end namespace
}

#endif
