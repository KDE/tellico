/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GUICOMBOBOXPROXY_H
#define TELLICO_GUICOMBOBOXPROXY_H

#include <kcombobox.h>
#include <kdebug.h>

#include <qguardedptr.h>
#include <qvaluelist.h>

namespace Tellico {
  namespace GUI {

/**
 * A combobox for mapping between combo items and any other class.
 *
 * Since QObjects can't be templates, it's not possibly to subclass KComboBox.
 * @ref The mapped object for the @ref currentItem() is available using @ref currentData().
 *
 * Important: Since this is not a QObject, it must be properly deleted!
 *
 * @author Robby Stephenson
 */
template <class T>
class ComboBoxProxy {
public:
  ComboBoxProxy(QWidget* parent_);

  ~ComboBoxProxy() {}

  KComboBox* comboBox() const { return m_comboBox; }

  const T& currentData() const;
  void insertItem(const QString& string, const T& datum, int index = -1);
  void insertItems(const QStringList& strings, const QValueList<T>& data, int index = -1);
  void setCurrentItem(const QString& item) { m_comboBox->setCurrentItem(item); }

private:
  QGuardedPtr<KComboBox> m_comboBox;
  QValueList<T> m_data;
};

  } // end namespace
} //end namespace

template <class T>
Tellico::GUI::ComboBoxProxy<T>::ComboBoxProxy(QWidget* parent_)
    : m_comboBox(new KComboBox(this, parent_)) {
  m_comboBox->setEditable(false);
}

template <class T>
void Tellico::GUI::ComboBoxProxy<T>::insertItem(const QString& s_, const T& t_, int idx_/* =-1 */) {
  if(!m_comboBox) {
    return;
  }
  m_comboBox->insertItem(s_, idx_);
  if(idx_ < 0) {
    m_data.push_back(t_);
  } else {
    while(idx_ > static_cast<int>(m_data.count())) {
      m_data.push_back(T());
    }
    m_data.insert(m_data.at(idx_), t_);
  }
}

template <class T>
void Tellico::GUI::ComboBoxProxy<T>::insertItems(const QStringList& s_, const QValueList<T>& t_, int idx_ /*=-1*/) {
  if(!m_comboBox) {
    return;
  }
  if(s_.count() != t_.count()) {
    kdWarning() << "ComboBoxProxy::insertItems() - must have equal number of items in list!" << endl;
    return;
  }

  for(uint i = 0; i < s_.count(); ++i) {
    insertItem(s_[i], t_[i], idx_+i);
  }
}

template <class T>
const T& Tellico::GUI::ComboBoxProxy<T>::currentData() const {
  if(!m_comboBox) {
    static T t; // inescapable
    return t;
  }
  return m_data[m_comboBox->currentItem()];
}

#endif
