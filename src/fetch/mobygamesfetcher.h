/***************************************************************************
    Copyright (C) 2019-2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_MOBYGAMESFETCHER_H
#define TELLICO_MOBYGAMESFETCHER_H

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

class MobyGamesFetcherTest;
namespace Tellico {
  namespace GUI {
    class ComboBox;
  }
  namespace Fetch {

/**
 * A fetcher for mobygames.com
 *
 * @author Robby Stephenson
 */
class MobyGamesFetcher : public Fetcher {
Q_OBJECT

friend class ::MobyGamesFetcherTest;

public:
  /**
   */
  MobyGamesFetcher(QObject* parent);
  /**
   */
  virtual ~MobyGamesFetcher();

  /**
   */
  virtual QString source() const override;
  virtual QString attribution() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual bool canSearch(FetchKey k) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return MobyGames; }
  virtual bool canFetch(int type) const override;
  virtual void readConfigHook(const KConfigGroup& config) override;
  virtual void continueSearch() override;

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const MobyGamesFetcher* fetcher = nullptr);
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
  void populateHashes();

private:
  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  Data::EntryList createEntries(Data::CollPtr coll, const QJsonObject& obj);

  // honor throttle limit for the API
  void markTime();
  // update cached data
  void updatePlatforms();

  enum ImageSize {
    SmallImage=0, // small is really the thumb size
    MediumImage=1,
    LargeImage=2,
    NoImage=3
  };

  bool m_started;
  ImageSize m_imageSize;

  QString m_apiKey;
  QHash<uint, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;
  QElapsedTimer m_idleTime;
  int m_requestPlatformId;

  QHash<int, QString> m_esrbHash;
  // key is the mobygames platform id
  QHash<int, QString> m_platforms;
};

  } // end namespace
} // end namespace
#endif
