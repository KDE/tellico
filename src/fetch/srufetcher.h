/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef SRUFETCHER_H
#define SRUFETCHER_H

#include "fetcher.h"

#include <qcstring.h> // for QByteARray
#include <qintdict.h>
#include <qguardedptr.h>

namespace KIO {
  class Job;
}

namespace Bookcase {
  class XSLTHandler;

  namespace Fetch {

/**
 * A fetcher for SRU servers.
 * Right now, only MODS is supported.
 *
 * @author Robby Stephenson
 * @version $Id: srufetcher.h 725 2004-08-03 14:00:57Z robby $
 */
class SRUFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  SRUFetcher(Data::Collection* coll, QObject* parent, const char* name = 0);
  /**
   */
  virtual ~SRUFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  virtual void stop();
  virtual Data::Entry* fetchEntry(uint uid);

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);

private:
  void initXSLTHandler();
  void cleanUp();

  QByteArray m_data;
  QIntDict<SearchResult> m_results;
  QIntDict<Data::Entry> m_entries;
  QGuardedPtr<KIO::Job> m_job;
  XSLTHandler* m_xsltHandler;
  bool m_collMerged;
  bool m_started;
};

  } // end namespace
} // end namespace
#endif
