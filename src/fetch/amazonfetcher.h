/***************************************************************************
    copyright            : (C) 2004 by Robby Stephenson
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
namespace KIO {
  class Job;
}

class KLineEdit;
class KComboBox;

class QCheckBox;
class QLabel;

#include "fetcher.h"
#include "configwidget.h"

#include <kurl.h>
#include <kio/job.h>

#include <qcstring.h> // for QByteArray
#include <qintdict.h>
#include <qguardedptr.h>

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for Amazon.com.
 *
 * @author Robby Stephenson
 * @version $Id: amazonfetcher.h 1068 2005-02-03 02:09:57Z robby $
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
  virtual bool canFetch(Data::Collection::Type type) {
    return type == Data::Collection::Book
           || type == Data::Collection::Bibtex
           || type == Data::Collection::Album
           || type == Data::Collection::Video;
  }
  virtual void readConfig(KConfig* config, const QString& group);
  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent);

  struct SiteData {
    QString title;
    KURL url;
    QString locale;
    QString books;
    QString dvd;
    QString vhs;
    QString music;
    QString classical;
  };
  static const SiteData& siteData(Site site);

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, AmazonFetcher* fetcher = 0);

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

  void cleanUp();

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
  QIntDict<SearchResult> m_results;
  QIntDict<Data::Entry> m_entries;
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
