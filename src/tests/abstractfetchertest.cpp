/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#undef QT_NO_CAST_FROM_ASCII

#include "abstractfetchertest.h"

#include "../fetch/fetcherjob.h"

#include <QDebug>
#include <QNetworkInterface>

AbstractFetcherTest::AbstractFetcherTest() : QObject(), m_loop(this), m_hasNetwork(false) {
  foreach(const QNetworkInterface& net, QNetworkInterface::allInterfaces()) {
    if(net.flags().testFlag(QNetworkInterface::IsUp) && !net.flags().testFlag(QNetworkInterface::IsLoopBack)) {
//      qDebug() << net.humanReadableName();
      m_hasNetwork = true;
      break;
    }
  }
}

Tellico::Data::EntryList AbstractFetcherTest::doFetch(Tellico::Fetch::Fetcher::Ptr fetcher,
                                                      const Tellico::Fetch::FetchRequest& request,
                                                      int maxResults) {
  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), SLOT(slotResult(KJob*)));

  if(maxResults > 0) {
    job->setMaximumResults(maxResults);
  }

  job->start();
  m_loop.exec();
  return m_results;
}

void AbstractFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
