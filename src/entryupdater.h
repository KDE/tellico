/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
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

#include <qpair.h>
#include <qvaluelist.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryUpdater : public QObject {
Q_OBJECT
public:
  EntryUpdater(Data::CollPtr coll, Data::EntryVec entries, QObject* parent);
  EntryUpdater(const QString& fetcher, Data::CollPtr coll, Data::EntryVec entries, QObject* parent);
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
  typedef QValueList<UpdateResult> ResultList;

  void init();
  void handleResults();
  UpdateResult askUser(ResultList results);
  void mergeCurrent(Data::EntryPtr entry, bool overwrite);

  Data::CollPtr m_coll;
  Data::EntryVec m_entriesToUpdate;
  Data::EntryVec m_fetchedEntries;
  Data::EntryVec m_matchedEntries;
  Fetch::FetcherVec m_fetchers;
  int m_fetchIndex;
  int m_origEntryCount;
  ResultList m_results;
  bool m_cancelled : 1;
};

} // end namespace

#endif
