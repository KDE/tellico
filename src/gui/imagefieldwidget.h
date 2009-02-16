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

#ifndef TELLICO_IMAGEFIELDWIDGET_H
#define TELLICO_IMAGEFIELDWIDGET_H

#include "fieldwidget.h"
#include "../datavectors.h"

class QString;

namespace Tellico {
  namespace GUI {
    class ImageWidget;

/**
 * @author Robby Stephenson
 */
class ImageFieldWidget : public FieldWidget {
Q_OBJECT

public:
  ImageFieldWidget(Data::FieldPtr field, QWidget* parent);
  virtual ~ImageFieldWidget() {}

  virtual QString text() const;
  virtual void setText(const QString& text);

public slots:
  virtual void clear();

protected:
  virtual QWidget* widget();

private:
  ImageWidget* m_widget;
};

  } // end GUI namespace
} // end namespace
#endif
