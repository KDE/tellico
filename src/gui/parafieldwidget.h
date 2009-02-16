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

#ifndef TELLICO_PARAFIELDWIDGET_H
#define TELLICO_PARAFIELDWIDGET_H

#include "fieldwidget.h"
#include "../datavectors.h"

class KTextEdit;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class ParaFieldWidget : public FieldWidget {
Q_OBJECT

public:
  ParaFieldWidget(Data::FieldPtr field, QWidget* parent);
  virtual ~ParaFieldWidget() {}

  virtual QString text() const;
  virtual void setText(const QString& text);

public slots:
  virtual void clear();

protected:
  virtual QWidget* widget();

private:
  KTextEdit* m_textEdit;
};

  } // end GUI namespace
} // end namespace
#endif
