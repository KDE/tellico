/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ItunesFetcher_H
#define TELLICO_ItunesFetcher_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QPointer>

class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  namespace GUI {
    class ComboBox;
  }
  namespace Fetch {

/**
 * A fetcher for iTunes
 *
 * @author Robby Stephenson
 */
class ItunesFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  ItunesFetcher(QObject* parent);
  /**
   */
  virtual ~ItunesFetcher();

  /**
   */
  virtual QString source() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual bool canSearch(FetchKey k) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return Itunes; }
  virtual bool canFetch(int type) const override;
  virtual void readConfigHook(const KConfigGroup& config) override;
  virtual void saveConfigHook(KConfigGroup& config) override;

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const ItunesFetcher* fetcher = nullptr);
    virtual void saveConfigHook(KConfigGroup&) override;
    virtual QString preferredName() const override;
  private:
     GUI::ComboBox* m_imageCombo;
     GUI::ComboBox* m_countryCombo;
     bool m_multiDiscTracks;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  void populateEntry(Data::EntryPtr entry, const QJsonObject& obj);
  void populateEpisodes(Data::EntryPtr entry);
  void readTrackInfo(const QJsonObject& resultMap);

  bool m_started;

  QHash<uint, Data::EntryPtr> m_entries;
  QHash<int, Data::EntryPtr> m_collectionHash;
  // a hash into a list of tracks, per disc
  QHash<int, QList<QStringList> > m_trackList;
  QPointer<KIO::StoredTransferJob> m_job;
  bool m_isTV;
  bool m_multiDiscTracks;

  enum ImageSize {
    NoImage=0,
    SmallImage=1, // small is the default 100x100
    LargeImage=2, // large is 600x600
  };
  ImageSize m_imageSize;

  QString m_country;
};

  } // end namespace
} // end namespace
#endif
