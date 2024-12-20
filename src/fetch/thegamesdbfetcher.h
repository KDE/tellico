/***************************************************************************
    Copyright (C) 2012-2019 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_THEGAMESDBFETCHER_H
#define TELLICO_THEGAMESDBFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QPointer>

class QLineEdit;

class KJob;
namespace KIO {
  class StoredTransferJob;
}

class TheGamesDBFetcherTest;
namespace Tellico {
  namespace GUI {
    class ComboBox;
  }

  namespace Fetch {

/**
 * A fetcher for thegamesdb.net
 *
 * @author Robby Stephenson
 */
class TheGamesDBFetcher : public Fetcher {
Q_OBJECT

friend class ::TheGamesDBFetcherTest;

public:
  /**
   */
  TheGamesDBFetcher(QObject* parent);
  /**
   */
  virtual ~TheGamesDBFetcher();

  /**
   */
  virtual QString source() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual bool canSearch(FetchKey k) const override;
  virtual Type type() const override { return TheGamesDB; }
  virtual bool canFetch(int type) const override;
  virtual void readConfigHook(const KConfigGroup& config) override;

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const TheGamesDBFetcher* fetcher = nullptr);
    virtual void saveConfigHook(KConfigGroup&) override;
    virtual QString preferredName() const override;

  private:
    QLineEdit* m_apiKeyEdit;
    GUI::ComboBox* m_imageCombo;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);
  // read all cached data
  void loadCachedData();

private:
  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  void populateEntry(Data::EntryPtr entry, const QVariantMap& resultMap);
  void readPlatformList(const QVariantMap& platformMap);
  void readCoverList(const QVariantMap& platformMap);

  // right now, Tgdb has three data types for which the whole list must be read at once
  // caching the platforms in addition, helps the UpdateRequest
  enum TgdbDataType { Genre, Publisher, Developer, Platform };
  static QString dataFileName(TgdbDataType dataType);

  // update cached data
  void updateData(TgdbDataType dataType, const QByteArray& data);
  // download data list from Tgdb and update cache
  void readDataList(TgdbDataType dataType);
  void writeDataList(TgdbDataType dataType, const QByteArray& data);

  enum ImageSize {
    SmallImage=0, // small is really the thumb size
    MediumImage=1,
    LargeImage=2,
    NoImage=3
  };

  bool m_started;
  QString m_apiKey;
  ImageSize m_imageSize;

  QHash<uint, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;
  QHash<QString, QString> m_covers;
  QHash<int, QString> m_genres;
  QHash<int, QString> m_publishers;
  QHash<int, QString> m_developers;
  QHash<int, QString> m_platforms;
};

  } // end namespace
} // end namespace
#endif
