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

namespace Tellico {
  class XSLTHandler;
}
namespace KIO {
  class Job;
}

#include "fetcher.h"
#include "configwidget.h"

#include <qcstring.h> // for QByteARray
#include <qintdict.h>
#include <qguardedptr.h>

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for SRU servers.
 * Right now, only MODS is supported.
 *
 * @author Robby Stephenson
 * @version $Id: srufetcher.h 960 2004-11-18 05:41:01Z robby $
 */
class SRUFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  SRUFetcher(QObject* parent, const char* name = 0);
  /**
   */
  virtual ~SRUFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value, bool multiple);
  // only search title, person, isbn, or keyword. No Raw for now.
  virtual bool canSearch(FetchKey k) const { return k != FetchFirst && k != FetchLast && k != Raw; }
  virtual void stop();
  virtual Data::Entry* fetchEntry(uint uid);
  virtual Type type() const { return SRU; }
  virtual bool canFetch(Data::Collection::Type type) {
    return type == Data::Collection::Book || type == Data::Collection::Bibtex;
  }
  virtual void readConfig(KConfig*, const QString&) {};
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent);

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_);

    virtual void saveConfig(KConfig*) {}
  };

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
