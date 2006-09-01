/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
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
 * A fetcher for Amazon.com.
 *
 * @author Robby Stephenson
 */
class YahooFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  YahooFetcher(QObject* parent, const char* name = 0);
  /**
   */
  virtual ~YahooFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  // amazon can search title or person
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return Yahoo; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(KConfig* config, const QString& group);

  virtual void updateEntry(Data::EntryPtr entry);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const YahooFetcher* fetcher = 0);
    virtual void saveConfig(KConfig*) {}
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);

private:
  void initXSLTHandler();
  void getTracks(Data::EntryPtr entry);
  QString insertValue(const QString& str, const QString& value, uint pos);

  XSLTHandler* m_xsltHandler;
  int m_limit;

  QByteArray m_data;
  QMap<int, Data::EntryPtr> m_entries; // they get modified after collection is created, so can't be const
  QGuardedPtr<KIO::Job> m_job;

  bool m_started;
};

  } // end namespace
} // end namespace
#endif
