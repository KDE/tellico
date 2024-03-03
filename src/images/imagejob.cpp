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
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>

#include <QTimer>
#include <QFileInfo>
#include <QBuffer>
#include <QImageReader>

namespace {
  static const int IMAGEJOB_TIMEOUT = 5; // seconds
}

using Tellico::ImageJob;

ImageJob::ImageJob(const QUrl& url_, const QString& id_, bool quiet_) : KIO::Job()
    , m_url(url_), m_id(id_), m_linkOnly(false), m_quiet(quiet_) {
  QTimer::singleShot(0, this, &ImageJob::slotStart);
}

ImageJob::~ImageJob() {
}

QString ImageJob::errorString() const {
  // by default, KIO::Job returns an error string depending on the error code and
  // using errorText() as a url or file name. Instead, just set a full error text and use it
  return errorText();
}

const Tellico::Data::Image& ImageJob::image() const {
  return m_image;
}

void ImageJob::setLinkOnly(bool linkOnly_) {
  m_linkOnly = linkOnly_;
}

void ImageJob::setReferrer(const QUrl& referrer_) {
  m_referrer = referrer_;
}

void ImageJob::slotStart() {
  if(!m_url.isValid()) {
    setError(KIO::ERR_MALFORMED_URL);
    emitResult();
  } else if(m_url.isLocalFile()) {
    const QString fileName = m_url.toLocalFile();
    if(!QFileInfo(fileName).isReadable()) {
      setError(KIO::ERR_CANNOT_OPEN_FOR_READING);
      setErrorText(i18n("Tellico is unable to load the image - %1.", fileName));
    } else {
      m_image = Data::Image(fileName, m_id);
      if(m_image.isNull()) {
        setError(KIO::ERR_UNKNOWN);
        m_image = Data::Image::null;
      }
      if(m_linkOnly) {
        m_image.setLinkOnly(true);
        m_image.setID(m_url.url());
      }
    }
    emitResult();
  } else {
    KIO::JobFlags flags = KIO::DefaultFlags;
    if(m_quiet || !GUI::Proxy::widget()) {
      flags |= KIO::HideProgressInfo;
    }
    // non-local valid url
    // KIO::storedGet seems to handle Content-Encoding: gzip ok
    KIO::StoredTransferJob* getJob = KIO::storedGet(m_url, KIO::NoReload, flags);
    QObject::connect(getJob, &KJob::result, this, &ImageJob::getJobResult);
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
  const auto errorText = i18n("Tellico is unable to load the image - %1.", m_url.toDisplayString());
  KIO::StoredTransferJob* getJob = qobject_cast<KIO::StoredTransferJob*>(job_);
  if(!getJob || getJob->error()) {
    // error handling for subjob is handled by KCompositeJob
    setErrorText(errorText);
    emitResult();
    return;
  }

  // If we used the Image() c'tor that take a bytearray of data, I'm not sure how to
  // figure out the image format directly. Instead, write into a buffer and use QImageReader
  QByteArray data = getJob->data();
  QBuffer buffer(&data);
  buffer.open(QIODevice::ReadOnly);
  const auto format = QString::fromLatin1(QImageReader::imageFormat(&buffer));
  m_image = Data::Image(data, format, m_id);
  if(m_image.isNull()) {
    setErrorText(errorText);
    setError(KIO::ERR_UNKNOWN);
    m_image = Data::Image::null;
  } else {
    // if we can't write the input format, then change to one we can
    const auto outputFormat = Data::Image::outputFormat(m_image.format());
    if(m_image.format() != outputFormat) {
      m_image.setFormat(outputFormat);
      // recalculate m_id if necessary, since the format is included in the id
      if(m_id.isEmpty()) {
        m_image.calculateID();
      }
    }
    if(m_linkOnly) {
      m_image.setLinkOnly(true);
      m_image.setID(m_url.url());
    }
  }
  emitResult();
}

void ImageJob::getJobTimeout() {
  setError(KIO::ERR_SERVER_TIMEOUT);
  for(auto job : subjobs()) {
    job->kill(KIO::Job::EmitResult);
  }
}
