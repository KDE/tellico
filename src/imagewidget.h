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

#ifndef BOOKCASEIMAGEWIDGET_H
#define BOOKCASEIMAGEWIDGET_H

class QLabel;
class QResizeEvent;
class QMouseEvent;
class QDragEnterEvent;
class QDropEvent;

#include "imagefactory.h"

#include <qwidget.h>
#include <qpixmap.h>

namespace Bookcase {

/**
 * @author Robby Stephenson
 * @version $Id: imagewidget.h 459 2004-02-14 21:20:30Z robby $
 */
class ImageWidget : public QWidget {
Q_OBJECT

public:
  ImageWidget(QWidget* parent, const char* name = 0);
  ~ImageWidget() {}

  const QString& id() const { return m_image.id(); }
  void setImage(const QString& id);

public slots:
  void slotClear();

signals:
  void signalModified();

protected:
  virtual void resizeEvent(QResizeEvent* ev);
  virtual void mousePressEvent(QMouseEvent* ev);
  virtual void mouseMoveEvent(QMouseEvent* ev);
  virtual void dragEnterEvent(QDragEnterEvent* ev);
  virtual void dropEvent(QDropEvent* ev);

private slots:
  void slotGetImage();

private:
  void setImage(const Data::Image& image);
  void scale();

  Data::Image m_image;
  QPixmap m_pixmap;
  QPixmap m_scaled;
  QLabel* m_label;
  QPoint m_dragStart;
};

} // end namespace
#endif
