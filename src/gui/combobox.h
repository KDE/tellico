/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
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

#include <qvariant.h>
#include <qvaluelist.h>

class QString;

namespace Tellico {
  namespace GUI {

/**
 * A combobox for mapping a QVariant to each item.
 *
 * @author Robby Stephenson
 */
class ComboBox : public KComboBox {
public:
  ComboBox(QWidget* parent_);

  void clear();
  const QVariant& currentData() const;
  const QVariant& data(uint index) const;
  void insertItem(const QString& string, const QVariant& datum, int index = -1);
  void insertItems(const QStringList& strings, const QValueList<QVariant>& data, int index = -1);

  // set current item to match data
  void setCurrentData(const QVariant& data);

private:
  QValueList<QVariant> m_data;
};

  } // end namespace
} //end namespace

#endif
