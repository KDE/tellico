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
#include <KService>
#include <QDateTime>

#include "config.h"

class QLabel;
class QResizeEvent;
class QMouseEvent;
class QDragEnterEvent;
class QDropEvent;
class QCheckBox;
class QLabel;
class QToolButton;
class QMenu;
class KProgressDialog;
class KProcess;
#ifdef HAVE_KSANE
class KDialog;
namespace KSaneIface { class KSaneWidget; }
#endif
namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class ImageWidget : public QWidget {
Q_OBJECT

public:
  ImageWidget(QWidget* parent);
  virtual ~ImageWidget();

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
  void slotScanImage();
  void imageReady(QByteArray &data, int w, int h, int bpl, int f);
  void slotEditImage();
  void slotEditMenu(QAction* action);
  void slotFinished();

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
  QMenu* m_editMenu;
  QToolButton* m_edit;
  KService::Ptr m_editor;
  QString m_img;
  KProcess* m_editProcess;
  KProgressDialog* m_waitDlg;
  QDateTime m_editedFileDateTime;
#ifdef HAVE_KSANE
  KSaneIface::KSaneWidget* m_saneWidget;
  KDialog* m_saneDlg;
#endif
};

  } // end GUI namespace
} // end namespace
#endif
