/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_RATINGWIDGET_H
#define TELLICO_RATINGWIDGET_H

#include "../datavectors.h"

#include <KHBox>

#include <QList>
#include <QLabel>
#include <QPixmap>

namespace Tellico {
  namespace Data {
    class Field;
  }
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class RatingWidget : public KHBox {
Q_OBJECT

typedef QList<QLabel*> LabelList;

public:
  RatingWidget(Data::FieldPtr field, QWidget* parent);

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

  Data::FieldPtr m_field;
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
