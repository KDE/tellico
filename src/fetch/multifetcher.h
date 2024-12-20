/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_MULTIFETCHER_H
#define TELLICO_MULTIFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"
#include "../gui/kwidgetlister.h"

#include <QFrame>

namespace Tellico {

  namespace GUI {
    class ComboBox;
    class CollectionTypeCombo;
  }

  namespace Fetch {

/**
 * A fetcher for combining results from multiple other fetchers
 *
 * @author Robby Stephenson
 */
class MultiFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  MultiFetcher(QObject* parent);
  /**
   */
  virtual ~MultiFetcher();

  /**
   */
  virtual QString source() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual bool canSearch(FetchKey k) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return Multiple; }
  virtual bool canFetch(int type) const override;
  virtual void readConfigHook(const KConfigGroup& config) override;

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  class ConfigWidget;
  friend class ConfigWidget;
  class FetcherItemWidget;
  class FetcherListWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotResult(Tellico::Fetch::FetchResult* result);
  void slotDone();

private:
  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  void readSources() const;

  Data::EntryList m_entries;
  Data::EntryList m_matches;
  QHash<uint, Data::EntryPtr> m_entryHash;
  int m_collType;
  QStringList m_uuids;
  mutable QList<Fetcher::Ptr> m_fetchers;
  int m_fetcherIndex;
  int m_resultIndex;

  bool m_started;
};

class MultiFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const MultiFetcher* fetcher = nullptr);
  virtual void saveConfigHook(KConfigGroup&) override;
  virtual QString preferredName() const override;

private Q_SLOTS:
  void slotTypeChanged();

private:
  GUI::CollectionTypeCombo* m_collCombo;
  FetcherListWidget* m_listWidget;
};

class MultiFetcher::FetcherItemWidget : public QFrame {
Q_OBJECT

public:
  FetcherItemWidget(QWidget* parent);

  void setFetchers(const QList<Fetcher::Ptr>& fetchers);
  void setSource(Fetcher::Ptr fetcher);
  QString fetcherUuid() const;

Q_SIGNALS:
  void signalModified();

public Q_SLOTS:

private:
  GUI::ComboBox* m_fetcherCombo;
};

class MultiFetcher::FetcherListWidget : public KWidgetLister {
Q_OBJECT

public:
  FetcherListWidget(QWidget* parent);

  void setFetchers(const QList<Fetcher::Ptr>& fetchers);
  void setSources(const QList<Fetcher::Ptr>& fetchers);
  QStringList uuids() const;

Q_SIGNALS:
  void signalModified();

protected:
  virtual QWidget* createWidget(QWidget* parent) override;
  QList<Fetcher::Ptr> m_fetchers;
};

  } // end namespace
} // end namespace
#endif
