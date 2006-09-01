/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GUI_COLLECTIONTYPECOMBO_H
#define TELLICO_GUI_COLLECTIONTYPECOMBO_H

#include "combobox.h"

namespace Tellico {

namespace GUI {

class CollectionTypeCombo : public ComboBox {
public:
  CollectionTypeCombo(QWidget* parent);
  void reset();
  void setCurrentType(int type);
  int currentType() const { return currentData().toInt(); }
};

  }
}
#endif
