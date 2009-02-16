/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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

public slots:
  void slotResult(Tellico::Fetch::SearchResult* result);
  void slotCancel();

private slots:
  void slotStartNext();
  void slotDone();
  void slotCleanup();

private:
  typedef QPair<Fetch::SearchResult*, bool> UpdateResult;
  typedef QList<UpdateResult> ResultList;

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
