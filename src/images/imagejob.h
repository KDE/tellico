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

#ifndef TELLICO_IMAGEJOB_H
#define TELLICO_IMAGEJOB_H

#include <KIO/Job>

#include "image.h"

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class ImageJob : public KIO::Job {
Q_OBJECT

public:
  ImageJob(const QUrl& url, const QString& id = QString(), bool quiet=false, bool linkOnly=false);
  virtual ~ImageJob();

  const Data::Image& image() const;

private Q_SLOTS:
  void slotStart();
  void getJobResult(KJob* job);

private:
  QUrl m_url;
  QString m_id;
  bool m_linkOnly;
  Data::Image m_image;
};

} // end namespace


#endif
