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

#ifndef TELLICO_IMAGEWIDGET_H
#define TELLICO_IMAGEWIDGET_H

#include <kurl.h>

#include <QWidget>
#include <QPixmap>
#include <QPoint>

class QLabel;
class QResizeEvent;
class QMouseEvent;
class QDragEnterEvent;
class QDropEvent;
class QCheckBox;
class QLabel;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class ImageWidget : public QWidget {
Q_OBJECT

public:
  ImageWidget(QWidget* parent);
  virtual ~ImageWidget() {}

  const QString& id() const { return m_imageID; }
  void setImage(const QString& id);
  void setLinkOnlyChecked(bool l);

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
  void slotLinkOnlyClicked();

private:
  void scale();
  void loadImage(const KUrl& url);

  QString m_imageID;
  QPixmap m_pixmap;
  QPixmap m_scaled;
  QLabel* m_label;
  QCheckBox* m_cbLinkOnly;
  KUrl m_originalURL;
  QPoint m_dragStart;
};

  } // end GUI namespace
} // end namespace
#endif
