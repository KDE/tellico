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

#ifndef TELLICO_FETCH_VGCOLLECTFETCHER_H
#define TELLICO_FETCH_VGCOLLECTFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>

class QUrl;
class KJob;
namespace KIO {
  class Job;
  class StoredTransferJob;
}

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for vgcollect.com
 *
 * @author Robby Stephenson
 */
class VGCollectFetcher : public Fetcher {
Q_OBJECT

public:
  VGCollectFetcher(QObject* parent);
  virtual ~VGCollectFetcher();

  virtual QString source() const override;
  virtual bool isSearching() const override { return m_started; }
  // only keyword search
  virtual bool canSearch(FetchKey k) const override { return k == Keyword; }
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return VGCollect; }
  virtual bool canFetch(int type) const override;
  virtual void readConfigHook(const KConfigGroup& config) override;

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
  void parseEntry(Data::EntryPtr entry, const QString& str);
  void populateValue(Data::EntryPtr entry, const QString& header, const QString& value) const;

  QHash<uint, Data::EntryPtr> m_entries;
  QHash<uint, QUrl> m_matches;
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
};

class VGCollectFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent, const VGCollectFetcher* fetcher = nullptr);
  virtual void saveConfigHook(KConfigGroup&) override {}
  virtual QString preferredName() const override;
};

  } // end namespace
} // end namespace
#endif
