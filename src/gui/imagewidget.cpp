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

#include "imagewidget.h"
#include "../imagefactory.h"
#include "../image.h"
#include "../filehandler.h"
#include "../tellico_debug.h"

#include <kfiledialog.h>
#include <klocale.h>
#include <kbuttonbox.h>
#include <kurldrag.h>

#include <qwmatrix.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qdragobject.h>
#include <qapplication.h> // needed for drag distance

namespace {
  static const uint IMAGE_WIDGET_BUTTON_MARGIN = 8;
  static const uint IMAGE_WIDGET_IMAGE_MARGIN = 4;
}

using Tellico::GUI::ImageWidget;

ImageWidget::ImageWidget(QWidget* parent_, const char* name_) : QWidget(parent_, name_) {
  QHBoxLayout* l = new QHBoxLayout(this);
  l->setMargin(IMAGE_WIDGET_BUTTON_MARGIN);
  m_label = new QLabel(this);
  m_label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
  m_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  m_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  l->addWidget(m_label, 1);
  l->addSpacing(IMAGE_WIDGET_BUTTON_MARGIN);

  KButtonBox* box = new KButtonBox(this, Vertical);
  box->addStretch(1);
  box->addButton(i18n("Select Image..."), this, SLOT(slotGetImage()));
  box->addButton(i18n("Clear"), this, SLOT(slotClear()));
  box->addStretch(1);
  box->layout();

  l->addWidget(box);
  slotClear();

  // accept image drops
  setAcceptDrops(true);
}

void ImageWidget::setImage(const QString& id_) {
  if(id_.isEmpty()) {
    slotClear();
    return;
  }
  m_imageID = id_;
  m_pixmap = ImageFactory::pixmap(id_);
  m_scaled = QPixmap();
  scale();

  update();
}

void ImageWidget::slotClear() {
//  m_image = Data::Image();
  m_imageID = QString();
  m_pixmap = QPixmap();
  m_scaled = m_pixmap;

  m_label->setPixmap(m_scaled);
  update();
  emit signalModified();
}

void ImageWidget::scale() {
  int ww = m_label->width() - 2*IMAGE_WIDGET_IMAGE_MARGIN;
  int wh = m_label->height() - 2*IMAGE_WIDGET_IMAGE_MARGIN;
  int pw = m_pixmap.width();
  int ph = m_pixmap.height();

  if(ww < pw || wh < ph) {
    int newWidth, newHeight;
    if(pw*wh < ph*ww) {
      newWidth = static_cast<int>(static_cast<float>(pw)*wh/static_cast<float>(ph));
      newHeight = wh;
    } else {
      newWidth = ww;
      newHeight = static_cast<int>(static_cast<float>(ph)*ww/static_cast<float>(pw));
    }

    QWMatrix wm;
    wm.scale(static_cast<float>(newWidth)/pw, static_cast<float>(newHeight)/ph);
    m_scaled = m_pixmap.xForm(wm);
  } else {
    m_scaled = m_pixmap;
  }
  m_label->setPixmap(m_scaled);
}

void ImageWidget::resizeEvent(QResizeEvent *) {
  if(m_pixmap.isNull()) {
    return;
  }

  scale();
  update();
}

void ImageWidget::slotGetImage() {
  KURL url = KFileDialog::getImageOpenURL(QString::null, this);
  if(url.isEmpty() || !url.isValid()) {
    return;
  }

  const QString& id = ImageFactory::addImage(url);
  if(id != m_imageID) {
    setImage(id);
    emit signalModified();
  }
}

void ImageWidget::mousePressEvent(QMouseEvent* event_) {
  // Only interested in LMB
  if(event_->button() == Qt::LeftButton) {
    // Store the position of the mouse press.
    // check if position is inside the label
    if(m_label->geometry().contains(event_->pos())) {
      m_dragStart = event_->pos();
    } else {
      m_dragStart = QPoint();
    }
  }
}

void ImageWidget::mouseMoveEvent(QMouseEvent* event_) {
  int delay = QApplication::startDragDistance();
  // Only interested in LMB
  if(event_->state() & Qt::LeftButton) {
    // only allow drag is the image is non-null, and the drag start point isn't null and the user dragged far enough
    if(!m_imageID.isEmpty() && !m_dragStart.isNull() && (m_dragStart - event_->pos()).manhattanLength() > delay) {
      const Data::Image& img = ImageFactory::imageById(m_imageID);
      if(!img.isNull()) {
        QImageDrag* drag = new QImageDrag(img, this);
        drag->dragCopy();
      }
    }
  }
}

void ImageWidget::dragEnterEvent(QDragEnterEvent* event_) {
  event_->accept(KURLDrag::canDecode(event_) || QImageDrag::canDecode(event_) || QTextDrag::canDecode(event_));
}

void ImageWidget::dropEvent(QDropEvent* event_) {
  QImage image;
  KURL::List urls;
  QString text;

  if(QImageDrag::decode(event_, image)) {
    // Qt reads PNG data by default
    const QString& id = ImageFactory::addImage(image, QString::fromLatin1("PNG"));
    if(!id.isEmpty() && id != m_imageID) {
      setImage(id);
      emit signalModified();
    }
  } else if(KURLDrag::decode(event_, urls)) {
    if(urls.isEmpty()) {
      return;
    }
    // only care about the first one
    const KURL& url = urls[0];
    if(url.isEmpty() || !url.isValid()) {
      return;
    }
//    kdDebug() << "ImageWidget::dropEvent() - " << url.prettyURL() << endl;

    const QString& id = ImageFactory::addImage(url);
    if(!id.isEmpty() && id != m_imageID) {
      setImage(id);
      emit signalModified();
    }
  } else if(QTextDrag::decode(event_, text)) {
    KURL url(text);
    if(url.isEmpty() || !url.isValid()) {
      return;
    }

    const QString& id = ImageFactory::addImage(url);
    if(!id.isEmpty() && id != m_imageID) {
      setImage(id);
      emit signalModified();
    }
  }
}

#include "imagewidget.moc"
