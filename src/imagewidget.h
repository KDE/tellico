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
class QPaintEvent;
class QResizeEvent;

#include "imagefactory.h"

#include <qwidget.h>
#include <qpixmap.h>

namespace Bookcase {

/**
 * @author Robby Stephenson
 * @version $Id: imagewidget.h 386 2004-01-24 05:12:28Z robby $
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

private slots:
  void slotGetImage();

private:
  void setImage(const Data::Image& image);
  void scale();

  Data::Image m_image;
  QPixmap m_pixmap;
  QPixmap m_scaled;
  QLabel* m_label;
};

} // end namespace
#endif
