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

#ifndef TELLICO_ENTREZFETCHER_H
#define TELLICO_ENTREZFETCHER_H

namespace Tellico {
  class XSLTHandler;
}

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <qcstring.h> // for QByteArray
#include <qguardedptr.h>

namespace KIO {
  class Job;
}

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
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person || k == Keyword || k == Raw || k == PubmedID || k == DOI; }
  virtual void search(FetchKey key, const QString& value);
  virtual void continueSearch();
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return Entrez; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);
  virtual void updateEntry(Data::EntryPtr entry);
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  static StringMap customFields();

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const EntrezFetcher* fetcher=0);
    virtual void saveConfig(KConfigGroup& config);
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);

private:
  void initXSLTHandler();
  void doSummary();

  void searchResults();
  void summaryResults();

  enum Step {
    Begin,
    Search,
    Summary,
    Fetch
  };

  XSLTHandler* m_xsltHandler;
  QString m_dbname;

  int m_start;
  int m_total;

  QByteArray m_data;
  QMap<int, Data::EntryPtr> m_entries; // map from search result id to entry
  QMap<int, int> m_matches; // search result id to pubmed id
  QGuardedPtr<KIO::Job> m_job;

  QString m_queryKey;
  QString m_webEnv;
  Step m_step;

  bool m_started;
  QStringList m_fields;
};

  } // end namespace
} // end namespace

#endif
