/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef RATINGWIDGET_H
#define RATINGWIDGET_H

#include "../datavectors.h"

#include <qhbox.h>
#include <qptrlist.h>
#include <qlabel.h>
#include <qpixmap.h>

namespace Tellico {
  namespace Data {
    class Field;
  }
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class RatingWidget : public QHBox {
Q_OBJECT

typedef QPtrList<QLabel> LabelList;

public:
  RatingWidget(Data::FieldPtr field, QWidget* parent, const char* name = 0);

  void clear();
  QString text() const;
  void setText(const QString& text);
  void updateField(Data::FieldPtr field);

  static const QPixmap& pixmap(const QString& value);

public slots:
  void update();

signals:
  void modified();

protected:
  virtual void mousePressEvent(QMouseEvent* e);

private:
  void init();
  void updateBounds();

  Data::ConstFieldPtr m_field;
  LabelList m_widgets;

  int m_currIndex;
  int m_total;
  int m_min;
  int m_max;

  QPixmap m_pixOn;
  QPixmap m_pixOff;
};

  } // end GUI namespace
} // end namespace
#endif
