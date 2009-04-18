/***************************************************************************
    copyright            : (C) 2004-2008 by Robby Stephenson
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

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <kurl.h>

#include <QPointer>
#include <QLabel>

class KLineEdit;

class QCheckBox;
class QLabel;

class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {

  class XSLTHandler;
  namespace GUI {
    class ComboBox;
  }

  namespace Fetch {

/**
 * A fetcher for Amazon.com.
 *
 * @author Robby Stephenson
 */
class AmazonFetcher : public Fetcher {
Q_OBJECT

public:
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

  AmazonFetcher(Site site, QObject* parent);
  virtual ~AmazonFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void continueSearch();
  // amazon can search title, person, isbn, or keyword. No Raw for now.
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person || k == ISBN || k == UPC || k == Keyword; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return Amazon; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual void updateEntry(Data::EntryPtr entry);

  struct SiteData {
    QString title;
    KUrl url;
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
  void slotComplete(KJob* job);

private:
  virtual void search(FetchKey key, const QString& value);
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

  int m_page;
  int m_total;
  int m_numResults;
  QMap<int, Data::EntryPtr> m_entries; // they get modified after collection is created, so can't be const
  QPointer<KIO::StoredTransferJob> m_job;

  FetchKey m_key;
  QString m_value;
  bool m_started;
  QStringList m_fields;
};

class AmazonFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  ConfigWidget(QWidget* parent_, const AmazonFetcher* fetcher = 0);

  virtual void saveConfig(KConfigGroup& config);
  virtual QString preferredName() const;

private slots:
  void slotSiteChanged();

private:
  KLineEdit* m_assocEdit;
  GUI::ComboBox* m_siteCombo;
  GUI::ComboBox* m_imageCombo;
};

  } // end namespace
} // end namespace
#endif
