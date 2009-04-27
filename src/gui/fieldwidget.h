/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FIELDWIDGET_H
#define TELLICO_FIELDWIDGET_H

#include "../datavectors.h"

#include <QWidget>
#include <QRegExp>

class QLabel;
class QCheckBox;
class QString;

namespace Tellico {
  namespace Data {
    class Field;
  }
  namespace GUI {

/**
 * The FieldWidget class is a box that shows a label, then a widget which depends
 * on the field type, and then a checkbox for multiple editing.
 *
 * @author Robby Stephenson
 */
class FieldWidget : public QWidget {
Q_OBJECT

public:
  FieldWidget(Data::FieldPtr field, QWidget* parent);
  virtual ~FieldWidget() {}

  Data::FieldPtr field() const { return m_field; }
  virtual QString text() const = 0;
  void setText(const QString& text);
  void clear();

  int labelWidth() const;
  void setLabelWidth(int width);
  bool isEnabled();
  bool expands() const;
  void editMultiple(bool show);
  // calls updateFieldHook()
  void updateField(Data::FieldPtr oldField, Data::FieldPtr newField);

  // only used by LineFieldWidget, really
  virtual void addCompletionObjectItem(const QString&) {}

  // factory function
  static FieldWidget* create(Data::FieldPtr field, QWidget* parent);

public slots:
  virtual void insertDefault();
  void setEnabled(bool enabled);

signals:
  virtual void valueChanged();

protected slots:
  void checkModified();

protected:
  QLabel* label() { return m_label; } // needed so the URLField can handle clicks on the label
  virtual QWidget* widget() = 0;
  void registerWidget();
  virtual void setTextImpl(const QString& text) = 0;
  virtual void clearImpl() = 0;

  // not all widgets have to be updated when the field changes
  virtual void updateFieldHook(Data::FieldPtr, Data::FieldPtr) {}

  static const QRegExp s_semiColon;

private:
  Data::FieldPtr m_field;
  QLabel* m_label;
  QCheckBox* m_editMultiple;
  QString m_oldValue;

  bool m_expands;
};

  } // end GUI namespace
} // end namespace
#endif
