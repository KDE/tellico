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

#include <QTimer>
#include <QFileInfo>

using Tellico::ImageJob;

ImageJob::ImageJob(const QUrl& url_, const QString& id_, bool quiet_) : KIO::Job()
    , m_url(url_), m_id(id_) {
  KIO::JobFlags flags = KIO::DefaultFlags;
  if(quiet_) {
    flags |= KIO::HideProgressInfo;
  }
  QTimer::singleShot(0, this, SLOT(slotStart()));
}

ImageJob::~ImageJob() {
}

void ImageJob::slotStart() {
  if(!m_url.isValid()) {
    setError(KIO::ERR_MALFORMED_URL);
  } else if(m_url.isLocalFile()) {
    const QString fileName = m_url.toLocalFile();
    if(!QFileInfo(fileName).isReadable()) {
      setError(KIO::ERR_CANNOT_OPEN_FOR_READING);
      //TODO ok to use KIO default? i18n(errorOpen, target_);
      setErrorText(fileName);
    } else {
      m_image = Data::Image(fileName, m_id);
    }
  }
  emitResult();
}

const Tellico::Data::Image& ImageJob::image() const {
  return m_image;
}
