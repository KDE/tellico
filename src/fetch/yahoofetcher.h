/***************************************************************************
    copyright            : (C) 2006-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef YAHOOFETCHER_H
#define YAHOOFETCHER_H

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
class YahooFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  YahooFetcher(QObject* parent);
  /**
   */
  virtual ~YahooFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  virtual void continueSearch();
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return Yahoo; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual void updateEntry(Data::EntryPtr entry);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const YahooFetcher* fetcher = 0);
    virtual void saveConfig(KConfigGroup&) {}
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotComplete(KJob* job);

private:
  void initXSLTHandler();
  void doSearch();
  void getTracks(Data::EntryPtr entry);
  QString insertValue(const QString& str, const QString& value, int pos);

  XSLTHandler* m_xsltHandler;
  int m_limit;
  int m_start;
  int m_total;

  QMap<int, Data::EntryPtr> m_entries; // they get modified after collection is created, so can't be const
  QPointer<KIO::StoredTransferJob> m_job;

  FetchKey m_key;
  QString m_value;
  bool m_started;
};

  } // end namespace
} // end namespace
#endif
