/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GUI_NUMBERFIELDWIDGET_H
#define TELLICO_GUI_NUMBERFIELDWIDGET_H

#include "fieldwidget.h"
#include "../datavectors.h"

class KLineEdit;

namespace Tellico {
  namespace GUI {
    class SpinBox;

/**
 * @author Robby Stephenson
 */
class NumberFieldWidget : public FieldWidget {
Q_OBJECT

public:
  NumberFieldWidget(Data::FieldPtr field, QWidget* parent);
  virtual ~NumberFieldWidget() {}

  virtual QString text() const;
  virtual void setTextImpl(const QString& text);

public slots:
  virtual void clearImpl();

protected:
  virtual QWidget* widget();
  virtual void updateFieldHook(Data::FieldPtr oldField, Data::FieldPtr newField);

private:
  bool isSpinBox() const { return (m_spinBox != 0); }
  void initLineEdit();
  void initSpinBox();

  KLineEdit* m_lineEdit;
  SpinBox* m_spinBox;
};

  } // end GUI namespace
} // end namespace
#endif
