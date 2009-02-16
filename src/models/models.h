/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_MODELS_H
#define TELLICO_MODELS_H

#include <qnamespace.h>

namespace Tellico {

  enum ModelRole {
    RowCountRole = Qt::UserRole + 1,
    EntryPtrRole,
    FieldPtrRole,
    GroupPtrRole,
    SaveStateRole
  };

} // end namespace
#endif
