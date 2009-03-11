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

#include "imagewidget.h"
#include "../imagefactory.h"
#include "../image.h"
#include "../imageinfo.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"
#include "../tellico_utils.h"

#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <KPushButton>

#include <QMatrix>
#include <QLabel>
#include <QCheckBox>
#include <QApplication> // needed for drag distance
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDropEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>

namespace {
  static const uint IMAGE_WIDGET_BUTTON_MARGIN = 8;
  static const uint IMAGE_WIDGET_IMAGE_MARGIN = 4;
  static const uint MAX_UNSCALED_WIDTH = 640;
  static const uint MAX_UNSCALED_HEIGHT = 640;
}

using Tellico::GUI::ImageWidget;

ImageWidget::ImageWidget(QWidget* parent_) : QWidget(parent_) {
  QHBoxLayout* l = new QHBoxLayout(this);
  l->setMargin(IMAGE_WIDGET_BUTTON_MARGIN);
  m_label = new QLabel(this);
  m_label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
  m_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  m_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  l->addWidget(m_label, 1);
  l->addSpacing(IMAGE_WIDGET_BUTTON_MARGIN);

  QVBoxLayout* boxLayout = new QVBoxLayout();
  l->addLayout(boxLayout);

  boxLayout->addStretch(1);

  KPushButton* button1 = new KPushButton(i18n("Select Image..."), this);
  connect(button1, SIGNAL(clicked()), this, SLOT(slotGetImage()));
  boxLayout->addWidget(button1);

  KPushButton* button2 = new KPushButton(i18n("Clear"), this);
  connect(button2, SIGNAL(clicked()), this, SLOT(slotClear()));
  boxLayout->addWidget(button2);

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
  m_originalURL = link ? KUrl(id_) : KUrl();
  m_scaled = QPixmap();
  scale();

  update();
}

void ImageWidget::setLinkOnlyChecked(bool link_) {
  m_cbLinkOnly->setChecked(link_);
}

void ImageWidget::slotClear() {
  bool wasEmpty = m_imageID.isEmpty();
//  m_image = Data::Image();
  m_imageID.clear();
  m_pixmap = QPixmap();
  m_scaled = m_pixmap;
  m_originalURL.clear();

  m_label->setPixmap(m_scaled);
  m_cbLinkOnly->setChecked(false);
  m_cbLinkOnly->setEnabled(true);
  update();
  if(!wasEmpty) {
    emit signalModified();
  }
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

    QMatrix wm;
    wm.scale(static_cast<double>(newWidth)/pw, static_cast<double>(newHeight)/ph);
    m_scaled = m_pixmap.transformed(wm, Qt::SmoothTransformation);
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
  KUrl url = KFileDialog::getImageOpenUrl(KUrl(), this);
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
  const QString& id = ImageFactory::addImage(m_originalURL, false, KUrl(), link);
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
  if(event_->buttons() & Qt::LeftButton) {
    // only allow drag if the image is non-null, and the drag start point isn't null and the user dragged far enough
    if(!m_imageID.isEmpty() && !m_dragStart.isNull() && (m_dragStart - event_->pos()).manhattanLength() > delay) {
      const Data::Image& img = ImageFactory::imageById(m_imageID);
      if(!img.isNull()) {
         QDrag* drag = new QDrag(this);
         QMimeData* mimeData = new QMimeData();
         mimeData->setImageData(img);
         drag->setMimeData(mimeData);
         drag->setPixmap(QPixmap::fromImage(img));
         drag->exec(Qt::CopyAction);
         event_->accept();
      }
    }
  }
}

void ImageWidget::dragEnterEvent(QDragEnterEvent* event_) {
  if(event_->mimeData()->hasImage() || event_->mimeData()->hasText()) {
    event_->acceptProposedAction();
  }
}

void ImageWidget::dropEvent(QDropEvent* event_) {
  GUI::CursorSaver cs;
  if(event_->mimeData()->hasImage()) {
    QVariant imageData = event_->mimeData()->imageData();
    // Qt reads PNG data by default
    const QString& id = ImageFactory::addImage(qvariant_cast<QPixmap>(imageData), QLatin1String("PNG"));
    if(!id.isEmpty() && id != m_imageID) {
      setImage(id);
      emit signalModified();
    }
    event_->acceptProposedAction();
  } else if(event_->mimeData()->hasText()) {
    KUrl url(event_->mimeData()->text());
    if(!url.isEmpty() && url.isValid()) {
      loadImage(url);
      event_->acceptProposedAction();
    }
  }
}

void ImageWidget::loadImage(const KUrl& url_) {
  const bool link = m_cbLinkOnly->isChecked();

  GUI::CursorSaver cs;
  // if we're linking only, then we want the image id to be the same as the url
  const QString& id = ImageFactory::addImage(url_, false, KUrl(), link);
  if(id != m_imageID) {
    setImage(id);
    emit signalModified();
  }
  // at the end, cause setImage() resets it
  m_originalURL = url_;
  m_cbLinkOnly->setEnabled(true);
}

#include "imagewidget.moc"
