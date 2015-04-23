/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
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
    CA = 5,
    CN = 6,
    ES = 7,
    IT = 8
  };

  enum ImageSize {
    SmallImage=0,
    MediumImage=1,
    LargeImage=2,
    NoImage=3
  };

  AmazonFetcher(QObject* parent);
  virtual ~AmazonFetcher();

  virtual QString source() const;
  virtual QString attribution() const;
  virtual bool isSearching() const { return m_started; }
  virtual void continueSearch();
  // amazon can search title, person, isbn, or keyword. No Raw for now.
  virtual bool canSearch(FetchKey k) const;
  virtual void stop();
  virtual Data::EntryPtr fetchEntryHook(uint uid);
  virtual Type type() const { return Amazon; }
  virtual bool canFetch(int type) const;

  struct SiteData {
    QString title;
    KUrl url;
  };
  static const SiteData& siteData(int site);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const ;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private slots:
  void slotComplete(KJob* job);

private:
  virtual void search();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);
  virtual void readConfigHook(const KConfigGroup& config);
  void initXSLTHandler();
  void doSearch();
  void parseTitle(Data::EntryPtr entry, int collType);
  bool parseTitleToken(Data::EntryPtr entry, const QString& token);
  QString secretKey() const;

  XSLTHandler* m_xsltHandler;
  Site m_site;
  ImageSize m_imageSize;

  QString m_access;
  QString m_assoc;
  // mutable so that secretKey() can be const
  mutable QByteArray m_amazonKey;
  bool m_addLinkField;
  int m_limit;
  int m_countOffset;

  int m_page;
  int m_total;
  int m_numResults;
  QHash<int, Data::EntryPtr> m_entries; // they get modified after collection is created, so can't be const
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
};

class AmazonFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const AmazonFetcher* fetcher = 0);

  virtual void saveConfigHook(KConfigGroup& config);
  virtual QString preferredName() const;

private slots:
  void slotSiteChanged();

private:
  KLineEdit* m_accessEdit;
  KLineEdit* m_secretKeyEdit;
  KLineEdit* m_assocEdit;
  GUI::ComboBox* m_siteCombo;
  GUI::ComboBox* m_imageCombo;
};

  } // end namespace
} // end namespace
#endif
