/***************************************************************************
    copyright            : (C) 2001-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "combobox.h"

#include <kdebug.h>

using Tellico::GUI::ComboBox;

ComboBox::ComboBox(QWidget* parent_) : KComboBox(parent_) {
}

void ComboBox::addItems(const QStringList& strings_, const QList<QVariant>& variants_) {
  if(strings_.count() != variants_.count()) {
    kWarning() << "ComboBox::insertItems() - must have equal number of items in list!";
    return;
  }

  for(int i = 0; i < strings_.count(); ++i) {
    addItem(strings_[i], variants_[i]);
  }
}

QVariant ComboBox::currentData(int role_) const {
  return itemData(currentIndex(), role_);
}

bool ComboBox::setCurrentData(const QVariant& data_, int role_) {
  bool success = false;
  for(int i = 0; i < count(); ++i) {
    if(itemData(i, role_) == data_) {
      setCurrentIndex(i);
      success = true;
      break;
    }
  }
  return success;
}

#include "combobox.moc"
