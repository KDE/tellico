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
  virtual QString source() const Q_DECL_OVERRIDE;
  virtual bool isSearching() const Q_DECL_OVERRIDE { return m_started; }
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return Douban; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;
  virtual void stop() Q_DECL_OVERRIDE;
  virtual Data::EntryPtr fetchEntryHook(uint uid) Q_DECL_OVERRIDE;
  virtual void readConfigHook(const KConfigGroup& config) Q_DECL_OVERRIDE;

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const Q_DECL_OVERRIDE;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const DoubanFetcher* fetcher = nullptr);
    virtual void saveConfigHook(KConfigGroup&) Q_DECL_OVERRIDE;
    virtual QString preferredName() const Q_DECL_OVERRIDE;
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
  virtual void search() Q_DECL_OVERRIDE;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) Q_DECL_OVERRIDE;
  virtual void resetSearch() {}
  void doSearch(const QString& term);
  Data::EntryPtr createEntry(const QVariantMap& resultMap);
  void populateBookEntry(Data::EntryPtr entry, const QVariantMap& resultMap);
  void populateVideoEntry(Data::EntryPtr entry, const QVariantMap& resultMap);
  void populateMusicEntry(Data::EntryPtr entry, const QVariantMap& resultMap);
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
