/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include "imagejob.h"
#include "../utils/guiproxy.h"
#include "../utils/tellico_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>

#include <QTimer>
#include <QFileInfo>
#include <QBuffer>
#include <QImageReader>
#include <QtConcurrent>

namespace {
  static const int IMAGEJOB_TIMEOUT = 5; // seconds
}

using Tellico::ImageJob;

ImageJob::ImageJob(const QUrl& url_, const QString& id_, bool quiet_) : KIO::Job(), m_quiet(quiet_) {
  m_info.url = url_;
  m_info.id = id_;
  QTimer::singleShot(0, this, &ImageJob::slotStart);
}

ImageJob::~ImageJob() = default;

QString ImageJob::errorString() const {
  // by default, KIO::Job returns an error string depending on the error code and
  // using errorText() as a url or file name. Instead, just set a full error text and use it
  return errorText();
}

QUrl ImageJob::url() const {
  return m_info.url;
}

bool ImageJob::linkOnly() const {
  return m_info.linkOnly;
}

const Tellico::Data::Image& ImageJob::image() const {
  return m_image;
}

void ImageJob::setLinkOnly(bool linkOnly_) {
  m_info.linkOnly = linkOnly_;
}

void ImageJob::setReferrer(const QUrl& referrer_) {
  m_referrer = referrer_;
}

void ImageJob::slotStart() {
  if(!m_info.url.isValid()) {
    setError(KIO::ERR_MALFORMED_URL);
    emitResult();
  } else if(m_info.url.isLocalFile()) {
    const QString fileName = m_info.url.toLocalFile();
    if(!QFileInfo(fileName).isReadable()) {
      setError(KIO::ERR_CANNOT_OPEN_FOR_READING);
      setErrorText(i18n("Tellico is unable to load the image - %1.", fileName));
      emitResult();
      return;
    }
    auto watcher = new QFutureWatcher<Tellico::Data::Image>(this);
    connect(watcher, &QFutureWatcher<Tellico::Data::Image>::finished, this, [this, watcher]() {
      watcher->deleteLater();
      m_image = watcher->result();
      if(m_image.isNull()) {
        setError(KIO::ERR_UNKNOWN);
        m_image = Data::Image::null;
      }
      emitResult();
    });

    QFuture<Tellico::Data::Image> future = QtConcurrent::run(loadFromFile, fileName, m_info);
    watcher->setFuture(future);
  } else {
    KIO::JobFlags flags = KIO::DefaultFlags;
    if(m_quiet || !GUI::Proxy::widget()) {
      flags |= KIO::HideProgressInfo;
    }
    // non-local valid url
    // KIO::storedGet seems to handle Content-Encoding: gzip ok
    KIO::StoredTransferJob* getJob = KIO::storedGet(m_info.url, KIO::NoReload, flags);
    QObject::connect(getJob, &KJob::result, this, &ImageJob::getJobResult);
    Tellico::addUserAgent(getJob);
    if(!m_referrer.isEmpty()) {
      getJob->addMetaData(QStringLiteral("referrer"), m_referrer.url());
    }
    if(!addSubjob(getJob)) {
      myDebug() << "ImageJob:: error adding subjob";
      emitResult();
    }
    QTimer::singleShot(IMAGEJOB_TIMEOUT * 1000, this, &ImageJob::getJobTimeout);
  }
}

void ImageJob::getJobResult(KJob* job_) {
  const auto errorText = i18n("Tellico is unable to load the image - %1.", m_info.url.toDisplayString());
  KIO::StoredTransferJob* getJob = qobject_cast<KIO::StoredTransferJob*>(job_);
  if(!getJob || getJob->error()) {
    // error handling for subjob is handled by KCompositeJob
    setErrorText(errorText);
    emitResult();
    return;
  }

  auto watcher = new QFutureWatcher<Tellico::Data::Image>(this);
  connect(watcher, &QFutureWatcher<Tellico::Data::Image>::finished, this, [this, watcher]() {
    watcher->deleteLater();
    m_image = watcher->result();
    if(m_image.isNull()) {
      setError(KIO::ERR_UNKNOWN);
      m_image = Data::Image::null;
    }
    emitResult();
  });

  QFuture<Tellico::Data::Image> future = QtConcurrent::run(loadFromData, getJob->data(), m_info);
  watcher->setFuture(future);
}

void ImageJob::getJobTimeout() {
  setError(KIO::ERR_SERVER_TIMEOUT);
  for(auto job : subjobs()) {
    job->kill(KIO::Job::EmitResult);
  }
}

Tellico::Data::Image ImageJob::loadFromFile(const QString& file_, const Info& info_) {
  Tellico::Data::Image img(file_, info_.id);
  if(info_.linkOnly) {
    img.setLinkOnly(true);
    img.setID(info_.url.url());
  }
  return img;
}

Tellico::Data::Image ImageJob::loadFromData(const QByteArray& data_, const Info& info_) {
  QByteArray data(data_);
  // If we used the Image() c'tor that take a bytearray of data, I'm not sure how to
  // figure out the image format directly. Instead, write into a buffer and use QImageReader
  QBuffer buffer(&data);
  buffer.open(QIODevice::ReadOnly);
  const auto format = QString::fromLatin1(QImageReader::imageFormat(&buffer));
  Tellico::Data::Image img(data, format, info_.id);

  // if we can't write the input format, then change to one we can
  const auto outputFormat = Data::Image::outputFormat(img.format());
  if(img.format() != outputFormat) {
    img.setFormat(outputFormat);
    // recalculate m_id if necessary, since the format is included in the id
    if(info_.id.isEmpty()) {
      img.calculateID();
    }
  }

  if(info_.linkOnly) {
    img.setLinkOnly(true);
    img.setID(info_.url.url());
  }
  return img;
}
