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

#ifndef PARAFIELDWIDGET_H
#define PARAFIELDWIDGET_H

#include "fieldwidget.h"
#include "../datavectors.h"

class QTextEdit;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class ParaFieldWidget : public FieldWidget {
Q_OBJECT

public:
  ParaFieldWidget(Data::FieldPtr field, QWidget* parent, const char* name=0);
  virtual ~ParaFieldWidget() {}

  virtual QString text() const;
  virtual void setText(const QString& text);

public slots:
  virtual void clear();

protected:
  virtual QWidget* widget();

private:
  QTextEdit* m_textEdit;
};

  } // end GUI namespace
} // end namespace
#endif
