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

#include <qptrlist.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryUpdater : public QObject {
Q_OBJECT
public:
  EntryUpdater(Data::CollPtr coll, Data::EntryVec entries, QObject* parent);
  EntryUpdater(const QString& fetcher, Data::CollPtr coll, Data::EntryVec entries, QObject* parent);

public slots:
  void slotResult(Tellico::Fetch::SearchResult* result);
  void slotDone();
  void slotCancel();

private:
  typedef QPtrList<Fetch::SearchResult> ResultList;

  void init();
  void cleanup();
  void handleResults();
  Data::EntryPtr askUser(ResultList results);
  void mergeCurrent(Data::EntryPtr entry);

  Data::CollPtr m_coll;
  Data::EntryVec m_entries;
  Fetch::FetcherVec m_fetchers;
  int m_fetchIndex;
  int m_origEntryCount;
  ResultList m_results;
  bool m_cancelled : 1;
};

} // end namespace

#endif
