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

#ifndef TELLICO_FETCH_ANIMENFOFETCHER_H
#define TELLICO_FETCH_ANIMENFOFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>

class KUrl;
class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for animenfo.com
 *
 * @author Robby Stephenson
 */
class AnimeNfoFetcher : public Fetcher {
Q_OBJECT

public:
  AnimeNfoFetcher(QObject* parent);
  virtual ~AnimeNfoFetcher() {}

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  // only keyword search
  virtual bool canSearch(FetchKey k) const { return k == Keyword; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return AnimeNfo; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual void updateEntry(Data::EntryPtr entry);

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_);
    virtual void saveConfig(KConfigGroup&) {}
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotComplete(KJob* job);

private:
  Data::EntryPtr parseEntry(const QString& str);

  QByteArray m_data;
  int m_total;
  QMap<int, Data::EntryPtr> m_entries;
  QMap<int, KUrl> m_matches;
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
//  QStringList m_fields;
};

  } // end namespace
} // end namespace
#endif
