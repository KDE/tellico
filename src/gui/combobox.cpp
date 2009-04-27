/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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
