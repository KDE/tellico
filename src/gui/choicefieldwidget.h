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

#ifndef CHOICEFIELDWIDGET_H
#define CHOICEFIELDWIDGET_H

#include "fieldwidget.h"
#include "../datavectors.h"

class KComboBox;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class ChoiceFieldWidget : public FieldWidget {
Q_OBJECT

public:
  ChoiceFieldWidget(Data::FieldPtr field, QWidget* parent);
  virtual ~ChoiceFieldWidget() {}

  virtual QString text() const;
  virtual void setTextImpl(const QString& text);

public slots:
  virtual void clearImpl();

protected:
  virtual QWidget* widget();
  virtual void updateFieldHook(Data::FieldPtr oldField, Data::FieldPtr newField);

private:
  KComboBox* m_comboBox;
};

  } // end GUI namespace
} // end namespace
#endif
