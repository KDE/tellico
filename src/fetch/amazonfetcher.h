/***************************************************************************
    copyright            : (C) 2004-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAZONFETCHER_H
#define AMAZONFETCHER_H

namespace Tellico {
  class XSLTHandler;
}

class KLineEdit;
class KComboBox;

class QCheckBox;
class QLabel;

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
class AmazonFetcher : public Fetcher {
Q_OBJECT

public:
  // must be in order, starting at 0
  enum Site {
    Unknown = -1,
    US = 0,
    UK = 1,
    DE = 2,
    JP = 3,
    FR = 4,
    CA = 5
  };

  enum ImageSize {
    SmallImage=0,
    MediumImage=1,
    LargeImage=2,
    NoImage=3
  };

  AmazonFetcher(Site site, QObject* parent, const char* name = 0);
  virtual ~AmazonFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  virtual void continueSearch();
  // amazon can search title, person, isbn, or keyword. No Raw for now.
  virtual bool canSearch(FetchKey k) const { return k != FetchFirst && k != FetchLast && k != Raw; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return Amazon; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(KConfig* config, const QString& group);

  virtual void updateEntry(Data::EntryPtr entry);

  struct SiteData {
    QString title;
    KURL url;
  };
  static const SiteData& siteData(int site);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const ;

  static StringMap customFields();

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);

private:
  void initXSLTHandler();
  void doSearch();
  void parseTitle(Data::EntryPtr entry, int collType);
  bool parseTitleToken(Data::EntryPtr entry, const QString& token);

  XSLTHandler* m_xsltHandler;
  Site m_site;
  ImageSize m_imageSize;

  QString m_access;
  QString m_assoc;
  bool m_addLinkField;
  int m_limit;
  int m_countOffset;

  QByteArray m_data;
  int m_page;
  int m_total;
  int m_numResults;
  QMap<int, Data::EntryPtr> m_entries; // they get modified after collection is created, so can't be const
  QGuardedPtr<KIO::Job> m_job;

  FetchKey m_key;
  QString m_value;
  bool m_started;
  QStringList m_fields;
};

class AmazonFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  ConfigWidget(QWidget* parent_, const AmazonFetcher* fetcher = 0);

  virtual void saveConfig(KConfig* config_);
  virtual QString preferredName() const;

private slots:
  void slotSiteChanged();

private:
  KLineEdit* m_assocEdit;
  KComboBox* m_siteCombo;
  KComboBox* m_imageCombo;
};

  } // end namespace
} // end namespace
#endif
