/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GOOGLESCHOLARFETCHER_H
#define TELLICO_GOOGLESCHOLARFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>
#include <QRegExp>

class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for Google Scholar
 *
 * @author Robby Stephenson
 */
class GoogleScholarFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  GoogleScholarFetcher(QObject* parent);
  /**
   */
  virtual ~GoogleScholarFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void continueSearch();
  // amazon can search title or person
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person || k == Keyword; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return GoogleScholar; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual void updateEntry(Data::EntryPtr entry);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const GoogleScholarFetcher* fetcher = 0);
    virtual void saveConfig(KConfigGroup&) {}
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotComplete(KJob* job);

private:
  virtual void search(FetchKey key, const QString& value);
  void doSearch();

  int m_limit;
  int m_start;
  int m_total;

  QMap<int, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;

  FetchKey m_key;
  QString m_value;
  bool m_started;

  QRegExp m_bibtexRx;
  bool m_cookieIsSet;
};

  } // end namespace
} // end namespace
#endif
