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
