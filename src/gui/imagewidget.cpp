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
#include "../tellico_utils.h"

#include <kfiledialog.h>
#include <klocale.h>
#include <kbuttonbox.h>
#include <kurldrag.h>
#include <kmessagebox.h>

#include <qwmatrix.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qdragobject.h>
#include <qapplication.h> // needed for drag distance

namespace {
  static const uint IMAGE_WIDGET_BUTTON_MARGIN = 8;
  static const uint IMAGE_WIDGET_IMAGE_MARGIN = 4;
  static const uint MAX_UNSCALED_WIDTH = 640;
  static const uint MAX_UNSCALED_HEIGHT = 640;
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

  QVBoxLayout* boxLayout = new QVBoxLayout(l);
  boxLayout->addStretch(1);

  KButtonBox* box = new KButtonBox(this, Vertical);
  box->addButton(i18n("Select Image..."), this, SLOT(slotGetImage()));
  box->addButton(i18n("Clear"), this, SLOT(slotClear()));
  box->layout();
  boxLayout->addWidget(box);

  boxLayout->addSpacing(8);
  m_cbLinkOnly = new QCheckBox(i18n("Save link only"), this);
  connect(m_cbLinkOnly, SIGNAL(clicked()), SLOT(slotLinkOnlyClicked()));
  boxLayout->addWidget(m_cbLinkOnly);

  boxLayout->addStretch(1);
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
  m_pixmap = ImageFactory::pixmap(id_, MAX_UNSCALED_WIDTH, MAX_UNSCALED_HEIGHT);
  const bool link = ImageFactory::imageInfo(id_).linkOnly;
  m_cbLinkOnly->setChecked(link);
  m_cbLinkOnly->setEnabled(link);
  // if we're using a link, then the original URL _is_ the id
  m_originalURL = link ? id_ : KURL();
  m_scaled = QPixmap();
  scale();

  update();
}

void ImageWidget::setLinkOnlyChecked(bool link_) {
  m_cbLinkOnly->setChecked(link_);
}

void ImageWidget::slotClear() {
//  m_image = Data::Image();
  m_imageID = QString();
  m_pixmap = QPixmap();
  m_scaled = m_pixmap;
  m_originalURL = KURL();

  m_label->setPixmap(m_scaled);
  m_cbLinkOnly->setChecked(false);
  m_cbLinkOnly->setEnabled(true);
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
      newWidth = static_cast<int>(static_cast<double>(pw)*wh/static_cast<double>(ph));
      newHeight = wh;
    } else {
      newWidth = ww;
      newHeight = static_cast<int>(static_cast<double>(ph)*ww/static_cast<double>(pw));
    }

    QWMatrix wm;
    wm.scale(static_cast<double>(newWidth)/pw, static_cast<double>(newHeight)/ph);
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
  loadImage(url);
}

void ImageWidget::slotLinkOnlyClicked() {
  if(m_imageID.isEmpty()) {
    // nothing to do, it has an empty image;
    return;
  }

  bool link = m_cbLinkOnly->isChecked();
  // if the user is trying to link and can't before there's no information about the url
  // the let him know that
  if(link && m_originalURL.isEmpty()) {
    KMessageBox::sorry(this, i18n("Saving a link is only possible for newly added images."));
    m_cbLinkOnly->setChecked(false);
    return;
  }
  // need to reset image id to be the original url
  // if we're linking only, then we want the image id to be the same as the url
  // so it needs to be added to the cache all over again
  // probably could do this without downloading the image all over again,
  // but I'm not going to do that right now
  const QString& id = ImageFactory::addImage(m_originalURL, false, KURL(), link);
  // same image, so no need to call setImage
  m_imageID = id;
  emit signalModified();
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

  GUI::CursorSaver cs(Qt::busyCursor);
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
    loadImage(url);
  } else if(QTextDrag::decode(event_, text)) {
    KURL url(text);
    if(url.isEmpty() || !url.isValid()) {
      return;
    }
    loadImage(url);
  }
}

void ImageWidget::loadImage(const KURL& url_) {
  const bool link = m_cbLinkOnly->isChecked();

  GUI::CursorSaver cs;
  // if we're linking only, then we want the image id to be the same as the url
  const QString& id = ImageFactory::addImage(url_, false, KURL(), link);
  if(id != m_imageID) {
    setImage(id);
    emit signalModified();
  }
  // at the end, cause setImage() resets it
  m_originalURL = url_;
  m_cbLinkOnly->setEnabled(true);
}

#include "imagewidget.moc"
