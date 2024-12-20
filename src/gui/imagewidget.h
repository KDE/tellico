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

#ifndef TELLICO_IMAGEWIDGET_H
#define TELLICO_IMAGEWIDGET_H

#include <KService>

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QPoint>
#include <QDateTime>
#include <QPointer>
#include <QUrl>

#include <config.h>
#ifdef HAVE_KSANE
#include <ksane_version.h>
#endif

class QLabel;
class QResizeEvent;
class QMouseEvent;
class QDragEnterEvent;
class QDropEvent;
class QCheckBox;
class QLabel;
class QToolButton;
class QMenu;
class QProgressDialog;

class KProcess;
#ifdef HAVE_KSANE
class KPageDialog;
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

public Q_SLOTS:
  void slotClear();

Q_SIGNALS:
  void signalModified();

protected:
  virtual void resizeEvent(QResizeEvent* ev) override;
  virtual void mousePressEvent(QMouseEvent* ev) override;
  virtual void mouseMoveEvent(QMouseEvent* ev) override;
  virtual void dragEnterEvent(QDragEnterEvent* ev) override;
  virtual void dropEvent(QDropEvent* ev) override;

private Q_SLOTS:
  void slotGetImage();
  void slotLinkOnlyClicked();
  void slotScanImage();
#ifdef HAVE_KSANE
#if KSANE_VERSION < QT_VERSION_CHECK(21,8,0)
  void imageReady(QByteArray &data, int w, int h, int bpl, int f);
#else
  void imageReady(const QImage& scannedImage);
#endif
#endif
  void slotEditImage();
  void slotEditMenu(QAction* action);
  void slotFinished();
  void cancelScan();
  void copyImage();
  void saveImageAs();

private:
  void contextMenuEvent(QContextMenuEvent* event) override;
  void scale();
  void loadImage(const QUrl& url);

  QString m_imageID;
  QPixmap m_pixmap;
  QPixmap m_scaled;
  QLabel* m_label;
  QCheckBox* m_cbLinkOnly;
  QUrl m_originalURL;
  QPoint m_dragStart;
  QMenu* m_editMenu;
  QToolButton* m_edit;
  KService::Ptr m_editor;
  QString m_img;
  KProcess* m_editProcess;
  QProgressDialog* m_waitDlg;
  QDateTime m_editedFileDateTime;
#ifdef HAVE_KSANE
  QPointer<KSaneIface::KSaneWidget> m_saneWidget;
  QPointer<KPageDialog> m_saneDlg;
  QString m_saneDevice;
  bool m_saneDeviceIsOpen;
#endif
};

  } // end GUI namespace
} // end namespace
#endif
