/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_FETCHERJOB_H
#define TELLICO_FETCH_FETCHERJOB_H

#include "fetcher.h"

#include <KJob>

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class FetcherJob : public KJob {
Q_OBJECT
public:
  FetcherJob(QObject* parent, Fetcher::Ptr fetcher, const FetchRequest& request);
  ~FetcherJob();

  virtual void start() override;

  QList<Tellico::Fetch::FetchResult*> results() { return m_results; }
  Data::EntryList entries();
  void setMaximumResults(int count);

protected:
  bool doKill() override;

private Q_SLOTS:
  void startSearch();
  void slotResult(Tellico::Fetch::FetchResult* result);
  void slotDone();

private:
  Fetcher::Ptr m_fetcher;
  FetchRequest m_request;
  QList<Tellico::Fetch::FetchResult*> m_results;
  int m_maximumResults;
};

  }
} // end namespace

#endif
