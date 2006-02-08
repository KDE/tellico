/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOLFIELDWIDGET_H
#define BOOLFIELDWIDGET_H

class QCheckBox;

#include "fieldwidget.h"

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class BoolFieldWidget : public FieldWidget {
Q_OBJECT

public:
  BoolFieldWidget(Data::FieldPtr field, QWidget* parent, const char* name=0);
  virtual ~BoolFieldWidget() {}

  virtual QString text() const;
  virtual void setText(const QString& text);

public slots:
  virtual void clear();

protected:
  virtual QWidget* widget();

private:
  QCheckBox* m_checkBox;
};

  } // end GUI namespace
} // end namespace
#endif
