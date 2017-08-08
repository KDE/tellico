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
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QTimer>
#include <QFileInfo>

using Tellico::ImageJob;

ImageJob::ImageJob(const QUrl& url_, const QString& id_, bool quiet_, bool linkOnly_) : KIO::Job()
    , m_url(url_), m_id(id_), m_linkOnly(linkOnly_) {
  KIO::JobFlags flags = KIO::DefaultFlags;
  if(quiet_) {
    flags |= KIO::HideProgressInfo;
  }
  QTimer::singleShot(0, this, SLOT(slotStart()));
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
        m_image = Data::Image::null;
      }
      if(m_linkOnly) {
        m_image.setLinkOnly(true);
        m_image.setID(m_url.url());
      }
    }
    emitResult();
  } else {
    // non-local valid url
    // KIO::storedGet seems to handle Content-Encoding: gzip ok
    KIO::StoredTransferJob* getJob = KIO::storedGet(m_url, KIO::NoReload);
    QObject::connect(getJob, &KJob::result, this, &ImageJob::getJobResult);
    addSubjob(getJob);
  }
}

void ImageJob::getJobResult(KJob* job_) {
  if(!job_ || job_->error()) {
    // error handling for subjob is handled by KCompositeJob
    return;
  }
  KIO::StoredTransferJob* getJob = qobject_cast<KIO::StoredTransferJob*>(job_);
  if(getJob) {
    // TODO: need to figure out appropriate way to know image format
    m_image = Data::Image(getJob->data(), QLatin1String("png"), m_id);
    if(m_id.isEmpty()) {
      m_image.calculateID();
    }
  }
  emitResult();
}
