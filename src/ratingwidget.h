/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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

#include <qhbox.h>
#include <qptrlist.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qstringlist.h>

namespace Tellico {
  namespace Data {
    class Field;
  }

/**
 * @author Robby Stephenson
 * @version $Id: ratingwidget.h 931 2004-10-24 23:15:11Z robby $
 */
class RatingWidget : public QHBox {
Q_OBJECT

typedef QPtrList<QLabel> PtrList;
typedef QPtrListIterator<QLabel> PtrListIterator;

public:
  RatingWidget(const Data::Field* field, QWidget* parent, const char* name = 0);

  void clear();
  const QString& text() const;
  void setText(const QString& text);
  void updateField(const Data::Field* field);

  static bool handleField(const Data::Field* const field);
  static const QPixmap& pixmap(const QString& value);

public slots:
  void update();

signals:
  void modified();

protected:
  virtual void mousePressEvent(QMouseEvent* e);

private:
  void init();
  void updateAllowed();

  const Data::Field* m_field;
  PtrList m_widgets;

  int m_currIndex;
  int m_total;
  QStringList m_allowed;

  QPixmap m_pixOn;
  QPixmap m_pixOff;
};

} // end namespace
#endif
