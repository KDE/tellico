/***************************************************************************
    copyright            : (C) 2004-2005 by Robby Stephenson
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

  /**
   */
  AmazonFetcher(Site site, QObject* parent, const char* name = 0);
  /**
   */
  virtual ~AmazonFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value, bool multiple);
  // amazon can search title, person, isbn, or keyword. No Raw for now.
  virtual bool canSearch(FetchKey k) const { return k != FetchFirst && k != FetchLast && k != Raw; }
  virtual void stop();
  virtual Data::Entry* fetchEntry(uint uid);
  virtual Type type() const { return Amazon; }
  virtual bool canFetch(int type) const;
  virtual void readConfig(KConfig* config, const QString& group);
  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const ;

  struct SiteData {
    QString title;
    KURL url;
    QString locale;
    QString books;
    QString dvd;
    QString vhs;
    QString music;
    QString classical;
    QString games;
  };
  static const SiteData& siteData(Site site);

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const AmazonFetcher* fetcher = 0);

    virtual void saveConfig(KConfig* config_);

  private:
    KLineEdit* m_assocEdit;
    KComboBox* m_siteCombo;
    KComboBox* m_imageCombo;
  };
  friend class ConfigWidget;

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);

private:
  static XSLTHandler* s_xsltHandler;
  static void initXSLTHandler();

  Site m_site;
  bool m_primaryMode;
  ImageSize m_imageSize;
  QString m_name;
  QString m_token;
  QString m_assoc;
  bool m_addLinkField;

  QByteArray m_data;
  int m_page;
  int m_total;
  QMap<int, Data::EntryPtr> m_entries; // they get modified after collection is created, so can't be const
  QStringList m_isbnList;
  QGuardedPtr<KIO::Job> m_job;

  FetchKey m_key;
  QString m_value;
  bool m_multiple;
  bool m_started;
};

  } // end namespace
} // end namespace
#endif
