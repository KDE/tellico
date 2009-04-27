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

#ifndef TELLICO_GUI_COMBOBOX_H
#define TELLICO_GUI_COMBOBOX_H

#include <kcombobox.h>

#include <QVariant>
#include <QList>

class QString;

namespace Tellico {
  namespace GUI {

/**
 * A combobox for mapping a QVariant to each item.
 *
 * @author Robby Stephenson
 */
class ComboBox : public KComboBox {
Q_OBJECT

public:
  ComboBox(QWidget* parent_);

  QVariant currentData(int role = Qt::UserRole) const;
  void addItems(const QStringList& strings, const QList<QVariant>& data);

  // set current item to match data
  bool setCurrentData(const QVariant& data, int role = Qt::UserRole);
};

  } // end namespace
} //end namespace

#endif
