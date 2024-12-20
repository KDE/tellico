/***************************************************************************
    Copyright (C) 2017-2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IGDBFETCHER_H
#define TELLICO_IGDBFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QLineEdit>
#include <QPointer>
#include <QElapsedTimer>

class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for igdb.com
 *
 * @author Robby Stephenson
 */
class IGDBFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  IGDBFetcher(QObject* parent);
  /**
   */
  virtual ~IGDBFetcher();

  /**
   */
  virtual QString source() const override;
  virtual QString attribution() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual bool canSearch(FetchKey k) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return IGDB; }
  virtual bool canFetch(int type) const override;
  virtual void readConfigHook(const KConfigGroup& config) override;
  virtual void saveConfigHook(KConfigGroup& config) override;
  virtual void continueSearch() override;

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
  void populateHashes();

private:
  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  void populateEntry(Data::EntryPtr entry, const QVariantMap& resultMap);
  void markTime() const;
  void checkAccessToken();

  QPointer<KIO::StoredTransferJob> igdbJob(const QUrl& url, const QString& query);

  // IGDB has three data types for which the whole list must be read at once
  enum IgdbDataType { Genre, Platform, Company };
  static QString dataFileName(IgdbDataType dataType);

  // update cached data
  void updateData(IgdbDataType dataType, const QByteArray& data);
  // download data list from Tgdb and update cache
  void readDataList(IgdbDataType dataType, const QList<int>& idList=QList<int>());

  bool m_started;
  mutable QElapsedTimer m_requestTimer;

  QString m_accessToken;
  QDateTime m_accessTokenExpires;
  QHash<uint, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;

  QHash<int, QString> m_genreHash;
  QHash<int, QString> m_platformHash;
  QHash<int, QString> m_esrbHash;
  QHash<int, QString> m_pegiHash;
  QHash<int, QString> m_companyHash;
};

class IGDBFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const IGDBFetcher* fetcher = nullptr);
  virtual void saveConfigHook(KConfigGroup&) override;
  virtual QString preferredName() const override;
};

  } // end namespace
} // end namespace
#endif
