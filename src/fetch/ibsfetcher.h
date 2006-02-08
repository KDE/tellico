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

#ifndef TELLICO_FETCH_IBSFETCHER_H
#define TELLICO_FETCH_IBSFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <kio/job.h>

#include <qcstring.h> // for QByteArray

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for animenfo.com
 *
 * @author Robby Stephenson
 */
class IBSFetcher : public Fetcher {
Q_OBJECT

public:
  IBSFetcher(QObject* parent, const char* name = 0);
  virtual ~IBSFetcher() {}

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  // can search title, person, isbn, or keyword. No UPC or Raw for now.
  virtual bool canSearch(FetchKey k) const { return k != FetchFirst && k != FetchLast && k!= UPC && k != Raw; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return IBS; }
  virtual bool canFetch(int type) const;
  virtual void readConfig(KConfig* config, const QString& group);

  virtual void updateEntry(Data::EntryPtr entry);

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_);
    virtual void saveConfig(KConfig*) {}
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);
  void slotCompleteISBN(KIO::Job* job);

private:
  Data::EntryPtr parseEntry(const QString& str);

  QString m_name;

  QByteArray m_data;
  int m_total;
  QMap<int, Data::EntryPtr> m_entries;
  QMap<int, KURL> m_matches;
  QGuardedPtr<KIO::Job> m_job;

  bool m_started;
//  QStringList m_fields;
};

  } // end namespace
} // end namespace
#endif
