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

#ifndef TELLICO_ENTRYUPDATER_H
#define TELLICO_ENTRYUPDATER_H

#include "datavectors.h"
#include "fetch/fetchmanager.h"

#include <QPair>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryUpdater : public QObject {
Q_OBJECT
public:
  EntryUpdater(Data::CollPtr coll, Data::EntryList entries, QObject* parent);
  EntryUpdater(const QString& fetcher, Data::CollPtr coll, Data::EntryList entries, QObject* parent);
  ~EntryUpdater();

  typedef QPair<Fetch::FetchResult*, bool> UpdateResult;
  typedef QList<UpdateResult> ResultList;

public Q_SLOTS:
  void slotResult(Tellico::Fetch::FetchResult* result);
  void slotCancel();

private Q_SLOTS:
  void slotStartNext();
  void slotDone();
  void slotCleanup();

private:
  void init();
  void handleResults();
  UpdateResult askUser(const ResultList& results);
  void mergeCurrent(Data::EntryPtr entry, bool overwrite);

  Data::CollPtr m_coll;
  Data::EntryList m_entriesToUpdate;
  Data::EntryList m_fetchedEntries;
  Data::EntryList m_matchedEntries;
  Fetch::FetcherVec m_fetchers;
  int m_fetchIndex;
  int m_origEntryCount;
  ResultList m_results;
  bool m_cancelled;
};

} // end namespace

#endif
