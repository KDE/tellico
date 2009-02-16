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

#ifndef TELLICO_DISCOGSFETCHER_H
#define TELLICO_DISCOGSFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <klineedit.h>

#include <QPointer>

class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {

  class XSLTHandler;

  namespace Fetch {

/**
 * A fetcher for discogs.com
 *
 * @author Robby Stephenson
 */
class DiscogsFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  DiscogsFetcher(QObject* parent);
  /**
   */
  virtual ~DiscogsFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  virtual void continueSearch();
  // amazon can search title or person
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person || k == Keyword; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return Discogs; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual void updateEntry(Data::EntryPtr entry);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  static StringMap customFields();

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const DiscogsFetcher* fetcher = 0);
    virtual void saveConfig(KConfigGroup&);
    virtual QString preferredName() const;
  private:
    KLineEdit *m_apiKeyEdit;
    QCheckBox* m_fetchImageCheck;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotComplete(KJob* job);

private:
  void initXSLTHandler();
  void doSearch();

  XSLTHandler* m_xsltHandler;
  int m_limit;
  int m_start;
  int m_total;

  QMap<int, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;

  FetchKey m_key;
  QString m_value;
  bool m_started;

  bool m_fetchImages;
  QString m_apiKey;
  QStringList m_fields;
};

  } // end namespace
} // end namespace
#endif
