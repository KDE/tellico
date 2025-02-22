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
#include "../utils/cursorsaver.h"
#include "../tellico_debug.h"

#include <KPageDialog>
#include <KLocalizedString>
#include <KMessageBox>
#include <KProcess>
#include <KApplicationTrader>
#include <KIO/DesktopExecParser>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KFileWidget>
#include <KRecentDirs>
#include <KStandardAction>

#include <QPushButton>
#include <QMenu>
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
#include <QMimeData>
#include <QProgressDialog>
#include <QFileDialog>
#include <QImageReader>
#include <QClipboard>

#ifdef HAVE_KSANE
#include <KSaneWidget>
#endif

Q_DECLARE_METATYPE(KService::Ptr)

namespace {
  static const uint IMAGE_WIDGET_BUTTON_MARGIN = 8;
  static const uint IMAGE_WIDGET_IMAGE_MARGIN = 4;
  static const uint MAX_UNSCALED_WIDTH = 640;
  static const uint MAX_UNSCALED_HEIGHT = 640;
}

using Tellico::GUI::ImageWidget;

ImageWidget::ImageWidget(QWidget* parent_) : QWidget(parent_), m_editMenu(nullptr),
  m_editProcess(nullptr), m_waitDlg(nullptr)
#ifdef HAVE_KSANE
  , m_saneWidget(nullptr), m_saneDlg(nullptr), m_saneDeviceIsOpen(false)
#endif
{
  QHBoxLayout* l = new QHBoxLayout(this);
  l->setContentsMargins(IMAGE_WIDGET_BUTTON_MARGIN,
                        IMAGE_WIDGET_BUTTON_MARGIN,
                        IMAGE_WIDGET_BUTTON_MARGIN,
                        IMAGE_WIDGET_BUTTON_MARGIN);
  m_label = new QLabel(this);
  m_label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
  m_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  m_label->setAlignment(Qt::AlignCenter);
  l->addWidget(m_label, 1);
  l->addSpacing(IMAGE_WIDGET_BUTTON_MARGIN);

  QVBoxLayout* boxLayout = new QVBoxLayout();
  l->addLayout(boxLayout);

  boxLayout->addStretch(1);

  QPushButton* button1 = new QPushButton(i18n("Select Image..."), this);
  button1->setIcon(QIcon::fromTheme(QStringLiteral("insert-image")));
  connect(button1, &QAbstractButton::clicked, this, &ImageWidget::slotGetImage);
  boxLayout->addWidget(button1);

  QPushButton* button2 = new QPushButton(i18n("Scan Image..."), this);
  button2->setIcon(QIcon::fromTheme(QStringLiteral("scanner")));
  connect(button2, &QAbstractButton::clicked, this, &ImageWidget::slotScanImage);
  boxLayout->addWidget(button2);
#ifndef HAVE_KSANE
  button2->setEnabled(false);
#endif

  m_edit = new QToolButton(this);
  m_edit->setText(i18n("Open With..."));
  m_edit->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  connect(m_edit, &QAbstractButton::clicked, this, &ImageWidget::slotEditImage);
  boxLayout->addWidget(m_edit);

  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("EditImage"));
  QString editor = config.readEntry("editor");
  m_editMenu = new QMenu(this);
  QActionGroup* grp = new QActionGroup(this);
  grp->setExclusive(true);
  QAction* selectedAction = nullptr;
  auto offers = KApplicationTrader::queryByMimeType(QStringLiteral("image/png"));
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
    connect(m_editMenu, &QMenu::triggered, this, &ImageWidget::slotEditMenu);
  } else {
    m_edit->setEnabled(false);
  }
  QPushButton* button4 = new QPushButton(i18nc("Clear image", "Clear"), this);
  button4->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear")));
  connect(button4, &QAbstractButton::clicked, this, &ImageWidget::slotClear);
  boxLayout->addWidget(button4);

  boxLayout->addSpacing(8);

  m_cbLinkOnly = new QCheckBox(i18n("Save link only"), this);
  connect(m_cbLinkOnly, &QAbstractButton::clicked, this, &ImageWidget::slotLinkOnlyClicked);
  boxLayout->addWidget(m_cbLinkOnly);

  boxLayout->addStretch(1);
  slotClear();

  // accept image drops
  setAcceptDrops(true);
}

ImageWidget::~ImageWidget() {
  if(m_editor) {
    KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("EditImage"));
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
  m_edit->setEnabled(true);
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
  const bool wasEmpty = m_imageID.isEmpty();
//  m_image = Data::Image();
  m_imageID.clear();
  m_pixmap = QPixmap();
  m_scaled = m_pixmap;
  m_originalURL.clear();

  m_label->setPixmap(m_scaled);
  m_cbLinkOnly->setChecked(false);
  m_cbLinkOnly->setEnabled(true);
  m_edit->setEnabled(false);
  update();
  if(!wasEmpty) {
    emit signalModified();
  }
}

void ImageWidget::contextMenuEvent(QContextMenuEvent* event_) {
  if(m_imageID.isEmpty() || m_pixmap.isNull()) {
    return;
  }

  QMenu menu(this);

  auto standardCopy = KStandardAction::copy(this, &ImageWidget::copyImage, &menu);
  standardCopy->setToolTip(QString()); // standard tool tip is
  menu.addAction(standardCopy);

  auto saveAs = KStandardAction::saveAs(this, &ImageWidget::saveImageAs, &menu);
  saveAs->setToolTip(QString());
  menu.addAction(saveAs);

  menu.exec(event_->globalPos());
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

    QTransform wm;
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
  QString filter;
  foreach(const QByteArray& ba, QImageReader::supportedImageFormats()) {
    if(!filter.isEmpty()) {
      filter += QLatin1Char(' ');
    }
    filter += QLatin1String("*.") + QString::fromLatin1(ba);
  }
  QStringList imageDirs = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
  QString imageStartDir = imageDirs.isEmpty() ? QString() : imageDirs.first();
  QString fileClass;
  QUrl startUrl = KFileWidget::getStartUrl(QUrl(QLatin1String("kfiledialog:///image") + imageStartDir), fileClass);
  const QUrl url = QFileDialog::getOpenFileUrl(this, QString(), startUrl, i18n("All Images (%1)", filter));
  if(url.isEmpty() || !url.isValid()) {
    return;
  }
  KRecentDirs::add(fileClass, url.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash).path());
  loadImage(url);
}

void ImageWidget::slotScanImage() {
#ifdef HAVE_KSANE
  if(!m_saneDlg) {
    m_saneDlg = new KPageDialog(this);
    m_saneWidget = new KSaneIface::KSaneWidget(m_saneDlg);
    m_saneDlg->addPage(m_saneWidget, QString());
    m_saneDlg->setStandardButtons(QDialogButtonBox::Cancel);
    m_saneDlg->setAttribute(Qt::WA_DeleteOnClose, false);
#if KSANE_VERSION < QT_VERSION_CHECK(21,8,0)
    connect(m_saneWidget.data(), &KSaneIface::KSaneWidget::imageReady,
#else
    connect(m_saneWidget.data(), &KSaneIface::KSaneWidget::scannedImageReady,
#endif
            this, &ImageWidget::imageReady);
    connect(m_saneDlg.data(), &QDialog::rejected,
            this, &ImageWidget::cancelScan);
  }
  if(m_saneDevice.isEmpty()) {
    m_saneDevice = m_saneWidget->selectDevice(this);
  }
  if(!m_saneDevice.isEmpty() && !m_saneDeviceIsOpen) {
    m_saneDeviceIsOpen = m_saneWidget->openDevice(m_saneDevice);
    if(!m_saneDeviceIsOpen) {
      KMessageBox::error(this, i18n("Opening the selected scanner failed."));
      m_saneDevice.clear();
    }
  }
  if(!m_saneDeviceIsOpen || m_saneDevice.isEmpty()) {
    return;
  }
  m_saneDlg->exec();
#endif
}

#ifdef HAVE_KSANE
#if KSANE_VERSION < QT_VERSION_CHECK(21,8,0)
void ImageWidget::imageReady(QByteArray& data, int w, int h, int bpl, int f) {
#else
void ImageWidget::imageReady(const QImage& scannedImage) {
#endif
   if(!m_saneWidget) {
     return;
   }
#if KSANE_VERSION < QT_VERSION_CHECK(21,8,0)
   QImage scannedImage = m_saneWidget->toQImage(data, w, h, bpl, static_cast<KSaneIface::KSaneWidget::ImageFormat>(f));
#endif

  QTemporaryFile temp(QDir::tempPath() + QLatin1String("/tellico_XXXXXX") + QLatin1String(".png"));
  if(temp.open()) {
    scannedImage.save(temp.fileName(), "PNG");
    // cannot link to the image
    m_cbLinkOnly->setChecked(false);
    loadImage(QUrl::fromLocalFile(temp.fileName()));
    m_originalURL.clear(); // don't allow linking to a temporary
    m_cbLinkOnly->setEnabled(false);
  } else {
    myWarning() << "Failed to open temp image file";
  }
  QTimer::singleShot(100, m_saneDlg.data(), &QDialog::accept);
}
#endif

void ImageWidget::slotEditImage() {
  if(m_imageID.isEmpty()) {
    return;
  }

  if(!m_editProcess) {
    m_editProcess = new KProcess(this);
    void (KProcess::* finished)(int, QProcess::ExitStatus) = &KProcess::finished;
    connect(m_editProcess, finished,
            this, &ImageWidget::slotFinished);
  }
  if(m_editor && m_editProcess->state() == QProcess::NotRunning) {
    QTemporaryFile temp(QDir::tempPath() + QLatin1String("/tellico_XXXXXX") + QLatin1String(".png"));
    if(temp.open()) {
      m_img = temp.fileName();
      const Data::Image& img = ImageFactory::imageById(m_imageID);
      img.save(m_img);
      m_editedFileDateTime = QFileInfo(m_img).lastModified();
      KIO::DesktopExecParser parser(*m_editor, QList<QUrl>() << QUrl::fromLocalFile(m_img));
      m_editProcess->setProgram(parser.resultingArguments());
      m_editProcess->start();
      if(!m_waitDlg) {
        m_waitDlg = new QProgressDialog(this);
        m_waitDlg->setCancelButton(nullptr);
        m_waitDlg->setLabelText(i18n("Opening image in %1...", m_editor->name()));
        m_waitDlg->setRange(0, 0);
      }
      m_waitDlg->exec();
    } else {
      myWarning() << "Failed to open temp image file";
    }
  }
}

void ImageWidget::slotFinished() {
  if(m_editedFileDateTime != QFileInfo(m_img).lastModified()) {
    // cannot link to the image
    m_cbLinkOnly->setChecked(false);
    loadImage(QUrl::fromLocalFile(m_img));
    m_originalURL.clear(); // don't allow linking to a temporary
    m_cbLinkOnly->setEnabled(false);
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
    KMessageBox::error(this, i18n("Saving a link is only possible for newly added images."));
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
    const QString& id = ImageFactory::addImage(qvariant_cast<QPixmap>(imageData), QStringLiteral("PNG"));
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

void ImageWidget::cancelScan() {
#ifdef HAVE_KSANE
  if(m_saneWidget) {
#if KSANE_VERSION < QT_VERSION_CHECK(22,4,0)
    m_saneWidget->scanCancel();
#else
    m_saneWidget->cancelScan();
#endif
  }
#endif
}

void ImageWidget::copyImage() {
  const Data::Image& img = ImageFactory::imageById(m_imageID);
  if(img.isNull()) {
    return;
  }

  QApplication::clipboard()->setImage(img, QClipboard::Clipboard);
  QApplication::clipboard()->setImage(img, QClipboard::Selection);
}

void ImageWidget::saveImageAs() {
  const Data::Image& img = ImageFactory::imageById(m_imageID);
  if(img.isNull()) {
    return;
  }

  QByteArray outputFormat = Data::Image::outputFormat(img.format());
  const QString filter = i18n("All Images (%1)", QLatin1String("*.") + QString::fromLatin1(outputFormat));
  const QUrl target = QFileDialog::getSaveFileUrl(this, QString(), QUrl(), filter);
  if(!target.isEmpty() && target.isValid()) {
    QString suffix = QFileInfo(target.fileName()).suffix();
    if(suffix.toLower().toUtf8() != outputFormat.toLower()) {
      outputFormat = Data::Image::outputFormat(suffix.toUtf8());
      myDebug() << "Writing image data as" << outputFormat;
    }
    const bool success = FileHandler::writeDataURL(target, Data::Image::byteArray(img, outputFormat));
    if(!success) {
      myDebug() << "Failed to write image to" << target.toDisplayString();
    }
  }
}
