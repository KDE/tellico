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
#include <KStandardDirs>
#include <KProgressDialog>
#include <KTemporaryFile>
#include <KProcess>
#include <KMimeTypeTrader>
#include <KRun>

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
  , m_saneWidget(0), m_saneDlg(0)
#endif
{
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

  KPushButton* button2 = new KPushButton(i18n("Scan Image..."), this);
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
  QString editor = config.readEntry("editor", "");
  m_editMenu = new QMenu(this);
  QActionGroup* grp = new QActionGroup(this);
  grp->setExclusive(true);
  QAction* selectedAction = 0;
  KService::List offers = KMimeTypeTrader::self()->query(QLatin1String("image/png"),
                                                         QLatin1String("Application"));
  foreach (KService::Ptr service, offers) {
    QAction* action = m_editMenu->addAction(KIcon(service->icon()), service->name());
    action->setCheckable(true);
    action->setData(QVariant::fromValue(service));
    grp->addAction(action);
    if (!selectedAction || editor == service->name()) {
      selectedAction = action;
    }
  }
  if (selectedAction) {
    selectedAction->setChecked(true);
    slotEditMenu(selectedAction);
    m_edit->setMenu(m_editMenu);
    connect(m_editMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotEditMenu(QAction*)));
  } else {
    m_edit->setEnabled(false);
  }
  KPushButton* button4 = new KPushButton(i18n("Clear"), this);
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
  KConfigGroup config(KGlobal::config(), "EditImage");
  config.writeEntry("editor", m_editor->name());
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

void ImageWidget::slotScanImage()
{
#ifdef HAVE_KSANE
  if (!m_saneWidget) {
    m_saneDlg = new KDialog(this);
    m_saneWidget = new KSaneIface::KSaneWidget(m_saneDlg);
    if (m_saneWidget->openDevice(QString()) == false) {
      QString dev = m_saneWidget->selectDevice(0);
      if (!dev.isEmpty()) {
        if (m_saneWidget->openDevice(dev) == false) {
          KMessageBox::sorry(0, i18n("Opening the selected scanner failed!"));
          dev.clear();
        }
      }
      if (dev.isEmpty()) {
        delete m_saneWidget;
        m_saneWidget = 0;
        delete m_saneDlg;
        m_saneDlg = 0;
        return;
      }
    }
    m_saneDlg->setMainWidget(m_saneWidget);
    m_saneDlg->setButtons(KDialog::Cancel);

    connect(m_saneWidget, SIGNAL(imageReady(QByteArray &, int, int, int, int)),
            this, SLOT(imageReady(QByteArray &, int, int, int, int)));
  }
  m_saneDlg->exec();
#endif
}

void ImageWidget::imageReady(QByteArray &data, int w, int h, int bpl, int f)
{
#ifdef HAVE_KSANE
  QImage scannedImage = m_saneWidget->toQImage(data, w, h, bpl,
          (KSaneIface::KSaneWidget::ImageFormat)f);
  scannedImage.setDotsPerMeterX(m_saneWidget->currentDPI() * (1000.0 / 25.4));
  scannedImage.setDotsPerMeterY(m_saneWidget->currentDPI() * (1000.0 / 25.4));
  KTemporaryFile temp;
  temp.setSuffix(QLatin1String(".png"));
  if (temp.open()) {
    scannedImage.save(temp.fileName());
    loadImage(temp.fileName());
  }
  m_saneDlg->close();
#endif
}

void ImageWidget::slotEditImage()
{
  if (!m_editProcess) {
    m_editProcess = new KProcess(this);
    connect(m_editProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(slotFinished()));
  }
  if (m_editProcess->state() == QProcess::NotRunning) {
    KTemporaryFile temp;
    temp.setSuffix(QLatin1String(".png"));
    if (temp.open()) {
      m_img = temp.fileName();
      const Data::Image& img = ImageFactory::imageById(m_imageID);
      img.save(m_img);
      m_editedFileDateTime = QFileInfo(m_img).lastModified();
      m_editProcess->setProgram(KRun::processDesktopExec(*m_editor, KUrl::List() << m_img));
      m_editProcess->start();
      if (!m_waitDlg) {
        m_waitDlg = new KProgressDialog(this);
        m_waitDlg->showCancelButton(false);
        m_waitDlg->setLabelText(i18n("Image open in %1...").arg(m_editor->name()));
        m_waitDlg->progressBar()->setRange(0, 0);
      }
      m_waitDlg->exec();
    }
  }
}

void ImageWidget::slotFinished()
{
  if (m_editedFileDateTime != QFileInfo(m_img).lastModified()) {
    loadImage(KUrl(m_img));
  }
  m_waitDlg->close();
}

void ImageWidget::slotEditMenu(QAction* action)
{
  m_editor = action->data().value<KService::Ptr>();
  m_edit->setIcon(KIcon(m_editor->icon()));
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
