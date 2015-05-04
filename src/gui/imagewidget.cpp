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

#include "imagewidget.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../images/imageinfo.h"
#include "../core/filehandler.h"
#include "../gui/cursorsaver.h"
#include "../tellico_debug.h"

#include <KFileDialog>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPushButton>
#include <KStandardDirs>
#include <KProgressDialog>
#include <KProcess>
#include <KMimeTypeTrader>
#include <KRun>
#include <KGlobal>

#include <QMenu>
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
#include <QToolButton>
#include <QActionGroup>
#include <QTimer>
#include <QSet>
#include <QDrag>
#include <QTemporaryFile>

#ifdef HAVE_KSANE
#include <libksane/ksane.h>
#endif

Q_DECLARE_METATYPE(KService::Ptr)

namespace {
  static const uint IMAGE_WIDGET_BUTTON_MARGIN = 8;
  static const uint IMAGE_WIDGET_IMAGE_MARGIN = 4;
  static const uint MAX_UNSCALED_WIDTH = 640;
  static const uint MAX_UNSCALED_HEIGHT = 640;
}

using Tellico::GUI::ImageWidget;

ImageWidget::ImageWidget(QWidget* parent_) : QWidget(parent_), m_editMenu(0),
  m_editProcess(0), m_waitDlg(0)
#ifdef HAVE_KSANE
  , m_saneWidget(0), m_saneDlg(0), m_saneDeviceIsOpen(false)
#endif
{
  QHBoxLayout* l = new QHBoxLayout(this);
  l->setMargin(IMAGE_WIDGET_BUTTON_MARGIN);
  m_label = new QLabel(this);
  m_label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
  m_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  m_label->setAlignment(Qt::AlignCenter);
  l->addWidget(m_label, 1);
  l->addSpacing(IMAGE_WIDGET_BUTTON_MARGIN);

  QVBoxLayout* boxLayout = new QVBoxLayout();
  l->addLayout(boxLayout);

  boxLayout->addStretch(1);

  KPushButton* button1 = new KPushButton(i18n("Select Image..."), this);
  button1->setIcon(QIcon::fromTheme(QLatin1String("insert-image")));
  connect(button1, SIGNAL(clicked()), this, SLOT(slotGetImage()));
  boxLayout->addWidget(button1);

  KPushButton* button2 = new KPushButton(i18n("Scan Image..."), this);
  button2->setIcon(QIcon::fromTheme(QLatin1String("scanner")));
  connect(button2, SIGNAL(clicked()), this, SLOT(slotScanImage()));
  boxLayout->addWidget(button2);
#ifndef HAVE_KSANE
  button2->setEnabled(false);
#endif

  m_edit = new QToolButton(this);
  m_edit->setText(i18n("Open With..."));
  m_edit->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  connect(m_edit, SIGNAL(clicked()), this, SLOT(slotEditImage()));
  boxLayout->addWidget(m_edit);

  KConfigGroup config(KGlobal::config(), "EditImage");
  QString editor = config.readEntry("editor");
  m_editMenu = new QMenu(this);
  QActionGroup* grp = new QActionGroup(this);
  grp->setExclusive(true);
  QAction* selectedAction = 0;
  KService::List offers = KMimeTypeTrader::self()->query(QLatin1String("image/png"),
                                                         QLatin1String("Application"));
  QSet<QString> offerNames;
  foreach(KService::Ptr service, offers) {
    if(offerNames.contains(service->name())) {
      continue;
    }
    offerNames.insert(service->name());
    QAction* action = m_editMenu->addAction(QIcon::fromTheme(service->icon()), service->name());
    action->setData(QVariant::fromValue(service));
    grp->addAction(action);
    if(!selectedAction || editor == service->name()) {
      selectedAction = action;
    }
  }
  if(selectedAction) {
    slotEditMenu(selectedAction);
    m_edit->setMenu(m_editMenu);
    connect(m_editMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotEditMenu(QAction*)));
  } else {
    m_edit->setEnabled(false);
  }
  KPushButton* button4 = new KPushButton(i18nc("Clear image", "Clear"), this);
  button4->setIcon(QIcon::fromTheme(QLatin1String("edit-clear")));
  connect(button4, SIGNAL(clicked()), this, SLOT(slotClear()));
  boxLayout->addWidget(button4);

  boxLayout->addSpacing(8);

  m_cbLinkOnly = new QCheckBox(i18n("Save link only"), this);
  connect(m_cbLinkOnly, SIGNAL(clicked()), SLOT(slotLinkOnlyClicked()));
  boxLayout->addWidget(m_cbLinkOnly);

  boxLayout->addStretch(1);
  slotClear();

  // accept image drops
  setAcceptDrops(true);
}

ImageWidget::~ImageWidget() {
  if(m_editor) {
    KConfigGroup config(KGlobal::config(), "EditImage");
    config.writeEntry("editor", m_editor->name());
  }
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
  m_cbLinkOnly->setEnabled(link); // user can't make a non;-linked image a linked image, so disable if not linked
  // if we're using a link, then the original URL _is_ the id
  m_originalURL = link ? QUrl(id_) : QUrl();
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
  QUrl url = KFileDialog::getImageOpenUrl(QUrl(), this);
  if(url.isEmpty() || !url.isValid()) {
    return;
  }
  loadImage(url);
}

void ImageWidget::slotScanImage() {
#ifdef HAVE_KSANE
  if(!m_saneDlg) {
    m_saneDlg = new KDialog(this);
    m_saneWidget = new KSaneIface::KSaneWidget(m_saneDlg);
    m_saneDlg->setMainWidget(m_saneWidget);
    m_saneDlg->setButtons(KDialog::Cancel);
    m_saneDlg->setAttribute(Qt::WA_DeleteOnClose, false);
    connect(m_saneWidget, SIGNAL(imageReady(QByteArray &, int, int, int, int)),
            SLOT(imageReady(QByteArray &, int, int, int, int)));
    // the dialog emits buttonClicked before it handles the actual cancel action
    connect(m_saneDlg, SIGNAL(buttonClicked(KDialog::ButtonCode)),
            SLOT(cancelScan(KDialog::ButtonCode)));
  }
  if(m_saneDevice.isEmpty()) {
    m_saneDevice = m_saneWidget->selectDevice(this);
  }
  if(!m_saneDevice.isEmpty() && !m_saneDeviceIsOpen) {
    m_saneDeviceIsOpen = m_saneWidget->openDevice(m_saneDevice);
    if(!m_saneDeviceIsOpen) {
      KMessageBox::sorry(this, i18n("Opening the selected scanner failed."));
      m_saneDevice.clear();
    }
  }
  if(!m_saneDeviceIsOpen || m_saneDevice.isEmpty()) {
    return;
  }
  m_saneDlg->exec();
#endif
}

void ImageWidget::imageReady(QByteArray& data, int w, int h, int bpl, int f) {
#ifdef HAVE_KSANE
  if(!m_saneWidget) {
    return;
  }
  QImage scannedImage = m_saneWidget->toQImage(data, w, h, bpl,
                                               static_cast<KSaneIface::KSaneWidget::ImageFormat>(f));
  QTemporaryFile temp(QDir::tempPath() + QLatin1String("/tellico_XXXXXX") + QLatin1String(".png"));
  if(temp.open()) {
    scannedImage.save(temp.fileName(), "PNG");
    loadImage(temp.fileName());
  } else {
    myWarning() << "Failed to open temp image file";
  }
  QTimer::singleShot(100, m_saneDlg, SLOT(accept()));
#else
  Q_UNUSED(data);
  Q_UNUSED(w);
  Q_UNUSED(h);
  Q_UNUSED(bpl);
  Q_UNUSED(f);
#endif
}

void ImageWidget::slotEditImage() {
  if(!m_editProcess) {
    m_editProcess = new KProcess(this);
    connect(m_editProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(slotFinished()));
  }
  if(m_editor && m_editProcess->state() == QProcess::NotRunning) {
    QTemporaryFile temp(QDir::tempPath() + QLatin1String("/tellico_XXXXXX") + QLatin1String(".png"));
    if(temp.open()) {
      m_img = temp.fileName();
      const Data::Image& img = ImageFactory::imageById(m_imageID);
      img.save(m_img);
      m_editedFileDateTime = QFileInfo(m_img).lastModified();
      m_editProcess->setProgram(KRun::processDesktopExec(*m_editor, QList<QUrl>() << QUrl::fromLocalFile(m_img)));
      m_editProcess->start();
      if(!m_waitDlg) {
        m_waitDlg = new KProgressDialog(this);
        m_waitDlg->showCancelButton(false);
        m_waitDlg->setLabelText(i18n("Opening image in %1...", m_editor->name()));
        m_waitDlg->progressBar()->setRange(0, 0);
      }
      m_waitDlg->exec();
    } else {
      myWarning() << "Failed to open temp image file";
    }
  }
}

void ImageWidget::slotFinished() {
  if(m_editedFileDateTime != QFileInfo(m_img).lastModified()) {
    loadImage(QUrl::fromLocalFile(m_img));
  }
  m_waitDlg->close();
}

void ImageWidget::slotEditMenu(QAction* action) {
  m_editor = action->data().value<KService::Ptr>();
  m_edit->setIcon(QIcon::fromTheme(m_editor->icon()));
}

void ImageWidget::slotLinkOnlyClicked() {
  if(m_imageID.isEmpty()) {
    // nothing to do, it has an empty image;
    return;
  }

  const bool link = m_cbLinkOnly->isChecked();
  // if the user is trying to link and can't before there's no information about the url
  // then let him know that
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
  const QString& id = ImageFactory::addImage(m_originalURL, false, QUrl(), link);
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
    QUrl url = QUrl::fromUserInput(event_->mimeData()->text());
    if(!url.isEmpty() && url.isValid()) {
      loadImage(url);
      event_->acceptProposedAction();
    }
  }
}

void ImageWidget::loadImage(const QUrl& url_) {
  const bool link = m_cbLinkOnly->isChecked();

  GUI::CursorSaver cs;
  // if we're linking only, then we want the image id to be the same as the url
  const QString& id = ImageFactory::addImage(url_, false, QUrl(), link);
  if(id != m_imageID) {
    setImage(id);
    emit signalModified();
  }
  // at the end, cause setImage() resets it
  m_originalURL = url_;
  m_cbLinkOnly->setEnabled(true);
}

void ImageWidget::cancelScan(KDialog::ButtonCode code_) {
  if(code_ != KDialog::Cancel) {
    return;
  }
#ifdef HAVE_KSANE
  if(m_saneWidget) {
    m_saneWidget->scanCancel();
  }
#endif
}

