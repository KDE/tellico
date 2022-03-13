/***************************************************************************
    Copyright (C) 2022 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_GAMINGHISTORYFETCHER_H
#define TELLICO_FETCH_GAMINGHISTORYFETCHER_H

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
 * A fetcher for arcade-history.com
 *
 * @author Robby Stephenson
 */
class GamingHistoryFetcher : public Fetcher {
Q_OBJECT

public:
  GamingHistoryFetcher(QObject* parent);
  virtual ~GamingHistoryFetcher();

  virtual QString source() const Q_DECL_OVERRIDE;
  virtual bool isSearching() const Q_DECL_OVERRIDE { return m_started; }
  // only keyword search
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE { return k == Keyword; }
  virtual void stop() Q_DECL_OVERRIDE;
  virtual Data::EntryPtr fetchEntryHook(uint uid) Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return GamingHistory; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;
  virtual void readConfigHook(const KConfigGroup& config) Q_DECL_OVERRIDE;

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const Q_DECL_OVERRIDE;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  virtual void search() Q_DECL_OVERRIDE;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) Q_DECL_OVERRIDE;
  void parseEntry(Data::EntryPtr entry, const QString& str);
  void parseSingleResult(Data::EntryPtr entry, const QUrl& url);
  void populatePlatform(Data::EntryPtr entry, const QString& platform);
  void populateYearIndex();

  QHash<uint, Data::EntryPtr> m_entries;
  QHash<uint, QUrl> m_matches;
  QHash<QString, int> m_yearIndex;
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
};

class GamingHistoryFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent, const GamingHistoryFetcher* fetcher = nullptr);
  virtual void saveConfigHook(KConfigGroup&) Q_DECL_OVERRIDE {}
  virtual QString preferredName() const Q_DECL_OVERRIDE;
};

  } // end namespace
} // end namespace
#endif
