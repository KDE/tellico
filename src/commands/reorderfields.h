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

#ifndef TELLICO_REORDERFIELDS_H
#define TELLICO_REORDERFIELDS_H

#include "../collection.h"
#include "../datavectors.h"

#include <kcommand.h>

#include <qguardedptr.h>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class ReorderFields : public KCommand {

public:
  ReorderFields(Data::Collection* coll, const Data::FieldVec& oldFields, const Data::FieldVec& newFields);

  virtual void execute();
  virtual void unexecute();
  virtual QString name() const;

private:
  QGuardedPtr<Data::Collection> m_coll;
  Data::FieldVec m_oldFields;
  Data::FieldVec m_newFields;
};

  } // end namespace
}

#endif
