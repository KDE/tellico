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

#ifndef TELLICO_BOOLFIELDWIDGET_H
#define TELLICO_BOOLFIELDWIDGET_H

#include "fieldwidget.h"
#include "../datavectors.h"

class QCheckBox;
class QString;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class BoolFieldWidget : public FieldWidget {
Q_OBJECT

public:
  BoolFieldWidget(Data::FieldPtr field, QWidget* parent);
  virtual ~BoolFieldWidget() {}

  virtual QString text() const;
  virtual void setTextImpl(const QString& text);

public slots:
  virtual void clearImpl();

protected:
  virtual QWidget* widget();

private:
  QCheckBox* m_checkBox;
};

  } // end GUI namespace
} // end namespace
#endif
