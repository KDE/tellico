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

#include "combobox.h"

#include <kdebug.h>

using Tellico::GUI::ComboBox;

ComboBox::ComboBox(QWidget* parent_) : KComboBox(parent_) {
  setEditable(false);
}

void ComboBox::clear() {
  KComboBox::clear();
  m_data.clear();
}

void ComboBox::insertItem(const QString& s_, const QVariant& t_, int idx_/* =-1 */) {
  KComboBox::insertItem(s_, idx_);
  if(idx_ < 0) {
    m_data.push_back(t_);
  } else {
    while(idx_ > static_cast<int>(m_data.count())) {
      m_data.push_back(QVariant());
    }
    m_data.insert(m_data.at(idx_), t_);
  }
}

void ComboBox::insertItems(const QStringList& s_, const QValueList<QVariant>& t_, int idx_ /*=-1*/) {
  if(s_.count() != t_.count()) {
    kdWarning() << "ComboBox::insertItems() - must have equal number of items in list!" << endl;
    return;
  }

  for(uint i = 0; i < s_.count(); ++i) {
    insertItem(s_[i], t_[i], idx_+i);
  }
}

const QVariant& ComboBox::currentData() const {
  return data(currentItem());
}

const QVariant& ComboBox::data(uint idx_) const {
  if(idx_ >= m_data.count()) {
    static QVariant t; // inescapable
    return t;
  }
  return m_data[idx_];
}

void ComboBox::setCurrentData(const QVariant& data_) {
  for(uint i = 0; i < m_data.count(); ++i) {
    if(m_data[i] == data_) {
      setCurrentItem(i);
      break;
    }
  }
}
