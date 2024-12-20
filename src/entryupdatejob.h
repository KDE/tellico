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

#ifndef TELLICO_ENTRYUPDATEJOB_H
#define TELLICO_ENTRYUPDATEJOB_H

#include "datavectors.h"
#include "fetch/fetcher.h"

#include <KJob>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryUpdateJob : public KJob {
Q_OBJECT
public:
  enum Mode { PerfectMatchOnly, GoodMatchOrBetter };
  EntryUpdateJob(QObject* parent, Data::EntryPtr entry, Fetch::Fetcher::Ptr fetcher, Mode mode = PerfectMatchOnly);

  virtual void start() override;

protected:
  bool doKill() override;

private Q_SLOTS:
  void startUpdate();
  void slotResult(Tellico::Fetch::FetchResult* result);
  void slotDone();

private:
  Data::EntryPtr m_entry;
  Fetch::Fetcher::Ptr m_fetcher;
  Mode m_mode;
  int m_bestMatchScore;
  Data::EntryPtr m_bestMatchEntry;
};

} // end namespace

#endif
