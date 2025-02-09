/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_DOUBANFETCHER_H
#define TELLICO_DOUBANFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QPointer>
#include <QVariantMap>

class KJob;
namespace KIO {
  class StoredTransferJob;
}
class DoubanFetcherTest;

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class DoubanFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  DoubanFetcher(QObject* parent);
  /**
   */
  virtual ~DoubanFetcher();

  /**
   */
  virtual QString source() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual bool canSearch(FetchKey k) const override;
  virtual Type type() const override { return Douban; }
  virtual bool canFetch(int type) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual void readConfigHook(const KConfigGroup& config) override;

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const DoubanFetcher* fetcher = nullptr);
    virtual void saveConfigHook(KConfigGroup&) override;
    virtual QString preferredName() const override;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);
  void slotCompleteISBN(KJob* job);

private:
  friend class ::DoubanFetcherTest;
  void setTestUrl1(const QUrl& url) { m_testUrl1 = url; }
  void setTestUrl2(const QUrl& url) { m_testUrl2 = url; }
  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  virtual void resetSearch() {}
  void doSearch(const QString& term);
  Data::EntryPtr createEntry(const QJsonObject& obj);
  void populateBookEntry(Data::EntryPtr entry, const QJsonObject& obj);
  void populateVideoEntry(Data::EntryPtr entry, const QJsonObject& obj);
  void populateMusicEntry(Data::EntryPtr entry, const QJsonObject& obj);
  void endJob(KIO::StoredTransferJob* job);

  bool m_started;

  QHash<uint, QUrl> m_matches;
  QHash<uint, Data::EntryPtr> m_entries;
  QList< QPointer<KIO::StoredTransferJob> > m_jobs;

  QUrl m_testUrl1;
  QUrl m_testUrl2;
};

  } // end namespace
} // end namespace
#endif
