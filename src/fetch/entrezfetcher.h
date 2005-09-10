/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_ENTREZFETCHER_H
#define TELLICO_ENTREZFETCHER_H

namespace Tellico {
  class XSLTHandler;
}

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <kurl.h>
#include <kio/job.h>

#include <qcstring.h> // for QByteArray
#include <qguardedptr.h>

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class EntrezFetcher : public Fetcher {
Q_OBJECT

public:
  EntrezFetcher(QObject* parent, const char* name=0);
  /**
   */
  virtual ~EntrezFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  // pubmed can search title, person, and keyword
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person || k == Keyword || k == Raw; }
  virtual void search(FetchKey key, const QString& value, bool multiple);
  virtual void stop();
  virtual Data::Entry* fetchEntry(uint uid);
  virtual Type type() const { return IMDB; }
  virtual bool canFetch(int type) const;
  virtual void readConfig(KConfig* config, const QString& group);
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_);

    virtual void saveConfig(KConfig*) {}
  };
  friend class ConfigWidget;

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);

private:
  static XSLTHandler* s_xsltHandler;
  static void initXSLTHandler();

  void searchResults();
  void summaryResults();

  enum Step {
    Begin,
    Search,
    Summary,
    Fetch
  };

  QString m_name;
  KURL m_url;
  QString m_dbname;
  QByteArray m_data;
  int m_total;
  QString m_queryKey;
  QString m_webEnv;
  QMap<int, Data::ConstEntryPtr> m_entries; // map from search result id to entry
  QMap<int, int> m_matches; // search result id to pubmed id
  QGuardedPtr<KIO::Job> m_job;
  Step m_step;
  bool m_started;
};

  } // end namespace
} // end namespace

#endif
