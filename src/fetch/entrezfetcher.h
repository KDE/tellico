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

#ifndef TELLICO_ENTREZFETCHER_H
#define TELLICO_ENTREZFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>

class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {

  class XSLTHandler;

  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class EntrezFetcher : public Fetcher {
Q_OBJECT

public:
  EntrezFetcher(QObject* parent);
  /**
   */
  virtual ~EntrezFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  // pubmed can search title, person, and keyword
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person || k == Keyword || k == Raw || k == PubmedID || k == DOI; }
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
  void slotComplete(KJob* job);

private:
  virtual void search(FetchKey key, const QString& value);
  void initXSLTHandler();
  void doSummary();

  void searchResults(const QByteArray& data);
  void summaryResults(const QByteArray& data);

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

  QMap<int, Data::EntryPtr> m_entries; // map from search result id to entry
  QMap<int, int> m_matches; // search result id to pubmed id
  QPointer<KIO::StoredTransferJob> m_job;

  QString m_queryKey;
  QString m_webEnv;
  Step m_step;

  bool m_started;
  QStringList m_fields;
};

  } // end namespace
} // end namespace

#endif
