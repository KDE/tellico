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
  void signalModified();

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
