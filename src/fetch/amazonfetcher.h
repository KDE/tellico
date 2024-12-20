/***************************************************************************
    Copyright (C) 2004-2020 Robby Stephenson <robby@periapsis.org>
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

#include <QUrl>

#include <QPointer>
#include <QLabel>

class QLineEdit;
class QCheckBox;
class QLabel;

class KJob;
namespace KIO {
  class StoredTransferJob;
}

class AmazonFetcherTest;
namespace Tellico {

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

friend class ::AmazonFetcherTest;

public:
  AmazonFetcher(QObject* parent);
  virtual ~AmazonFetcher();

  virtual QString source() const override;
  virtual QString attribution() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual void continueSearch() override;
  // amazon can search title, person, isbn, or keyword. No Raw for now.
  virtual bool canSearch(FetchKey k) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return Amazon; }
  virtual bool canFetch(int type) const override;

  struct SiteData {
    QString title;
    QByteArray host;
    QByteArray region;
    QString country;
    QString countryName;
  };
  static const SiteData& siteData(int site);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  virtual void readConfigHook(const KConfigGroup& config) override;
  void doSearch();
  QByteArray requestPayload(const FetchRequest& request);
  Data::CollPtr createCollection();
  void populateEntry(Data::EntryPtr entry, const QJsonObject& info);
  void parseTitle(Data::EntryPtr entry);
  bool parseTitleToken(Data::EntryPtr entry, const QString& token);

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
    IT = 8,
    BR = 9,
    AU = 10,
    IN = 11,
    MX = 12,
    TR = 13,
    SG = 14,
    AE = 15,
    XX = 16
  };

  enum ImageSize {
    SmallImage=0,
    MediumImage=1,
    LargeImage=2,
    NoImage=3
  };

  Site m_site;
  ImageSize m_imageSize;

  QString m_assoc;
  QString m_accessKey;
  QString m_secretKey;
  int m_limit;
  int m_countOffset;

  int m_page;
  int m_total;
  int m_numResults;
  QHash<uint, Data::EntryPtr> m_entries; // they get modified after collection is created, so can't be const
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
  QString m_testResultsFile;
  bool m_enableLog;
};

class AmazonFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const AmazonFetcher* fetcher = nullptr);

  virtual void saveConfigHook(KConfigGroup& config) override;
  virtual QString preferredName() const override;

private Q_SLOTS:
  void slotSiteChanged();

private:
  QLineEdit* m_accessEdit;
  QLineEdit* m_secretKeyEdit;
  QLineEdit* m_assocEdit;
  GUI::ComboBox* m_siteCombo;
  GUI::ComboBox* m_imageCombo;
};

  } // end namespace
} // end namespace
#endif
