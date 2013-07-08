/***************************************************************************
    Copyright (C) 2009-2013 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FREEBASEFETCHER_H
#define TELLICO_FREEBASEFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QPointer>
#include <QVariantList>

class KJob;
namespace KIO {
  class StoredTransferJob;
}
class KLineEdit;

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for freebase.com
 *
 * @author Robby Stephenson
 */
class FreebaseFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  FreebaseFetcher(QObject* parent);
  /**
   */
  virtual ~FreebaseFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void continueSearch();
  virtual bool canSearch(FetchKey k) const;
  virtual void stop();
  virtual Data::EntryPtr fetchEntryHook(uint uid);
  virtual Type type() const { return Freebase; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const FreebaseFetcher* fetcher = 0);
    virtual void saveConfigHook(KConfigGroup&);
    virtual QString preferredName() const;
  private:
    KLineEdit* m_apiKeyEdit;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private slots:
  void slotComplete(KJob* job);

private:
  static QString value(const QVariantMap& map, const char* name);
  static QString value(const QVariantMap& map, const char* object, const char* name);

  virtual void search();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);
  void doSearch();
  void endJob(KIO::StoredTransferJob* job);

  QVariantList bookQueries() const;
  QVariantList comicBookQueries() const;
  QVariantList movieQueries() const;
  QVariantList musicQueries() const;
  QVariantList videoGameQueries() const;
  QVariantList boardGameQueries() const;

  QHash<int, Data::EntryPtr> m_entries;
  QList< QPointer<KIO::StoredTransferJob> > m_jobs;

  bool m_started;
  QMap<uint, QVariant> m_cursors;
  QString m_apiKey;
};

  } // end namespace
} // end namespace
#endif
