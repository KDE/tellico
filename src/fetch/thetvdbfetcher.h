/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_THETVDBFETCHER_H
#define TELLICO_THETVDBFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QPointer>

class QLineEdit;
class KJob;
namespace KIO {
  class StoredTransferJob;
}
class TheTVDBFetcherTest;

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for thetvdb.com
 *
 * @author Robby Stephenson
 */
class TheTVDBFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  TheTVDBFetcher(QObject* parent);
  /**
   */
  virtual ~TheTVDBFetcher();

  /**
   */
  virtual QString source() const override;
  virtual QString attribution() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual bool canSearch(FetchKey k) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return TheTVDB; }
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

private:
  friend class ::TheTVDBFetcherTest;
  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  void populateEntry(Data::EntryPtr entry, const QVariantMap& resultMap, bool fullData);
  void populatePeople(Data::EntryPtr entry, const QJsonArray& castArray);
  void populateEpisodes(Data::EntryPtr entry, const QJsonArray& castArray);
  void checkAccessToken();
  void requestToken();
  void refreshToken();

  QPointer<KIO::StoredTransferJob> getJob(const QUrl& url, bool checkToken=true);

  bool m_started;

  QString m_apiKey;
  QString m_apiPin;
  QString m_accessToken;
  QDateTime m_accessTokenExpires;
  QHash<uint, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;
};

class TheTVDBFetcher::ConfigWidget : public Fetch::ConfigWidget {

public:
  explicit ConfigWidget(QWidget* parent_, const TheTVDBFetcher* fetcher = nullptr);
  virtual void saveConfigHook(KConfigGroup&) override;
  virtual QString preferredName() const override;

private:
  QLineEdit* m_apiPinEdit;
};

  } // end namespace
} // end namespace
#endif
