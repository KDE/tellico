/***************************************************************************
    copyright            : (C) 2007-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FETCH_CITEBASEFETCHER_H
#define TELLICO_FETCH_CITEBASEFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QPointer>

class KUrl;
class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class CitebaseFetcher : public Fetcher {
Q_OBJECT

public:
  CitebaseFetcher(QObject* parent);
  ~CitebaseFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);

  virtual bool canSearch(FetchKey k) const { return k == ArxivID; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return Citebase; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual void updateEntry(Data::EntryPtr entry);
  virtual void updateEntrySynchronous(Data::EntryPtr entry);

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const CitebaseFetcher* fetcher = 0);
    virtual void saveConfig(KConfigGroup& config);
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotComplete(KJob* job);

private:
  KUrl searchURL(FetchKey key, const QString& value) const;

  QMap<int, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;

  FetchKey m_key;
  QString m_value;
  bool m_started;
};

  }
}
#endif
