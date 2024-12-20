/***************************************************************************
    Copyright (C) 2009-2014 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_THEMOVIEDBFETCHER_H
#define TELLICO_THEMOVIEDBFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QPointer>
#include <QDate>

class QLineEdit;
class QSpinBox;

class KJob;
namespace KIO {
  class StoredTransferJob;
}
class TheMovieDBFetcherTest;

namespace Tellico {

  namespace GUI {
    class ComboBox;
  }

  namespace Fetch {

/**
 * A fetcher for themoviedb.org
 *
 * @author Robby Stephenson
 */
class TheMovieDBFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  TheMovieDBFetcher(QObject* parent);
  /**
   */
  virtual ~TheMovieDBFetcher();

  /**
   */
  virtual QString source() const override;
  virtual QString attribution() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual bool canSearch(FetchKey k) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return TheMovieDB; }
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
  friend class ::TheMovieDBFetcherTest;
  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  void populateEntry(Data::EntryPtr entry, const QVariantMap& resultMap, bool fullData);
  void readConfiguration();

  bool m_started;

  QString m_locale;
  QDate m_serverConfigDate;
  QString m_apiKey;
  QString m_imageBase;
  int m_numCast;

  QHash<uint, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;
};

class TheMovieDBFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const TheMovieDBFetcher* fetcher = nullptr);
  virtual void saveConfigHook(KConfigGroup&) override;
  virtual QString preferredName() const override;

private Q_SLOTS:
  void slotLangChanged();

private:
  QLineEdit* m_apiKeyEdit;
  GUI::ComboBox* m_langCombo;
  QSpinBox* m_numCast;
};

  } // end namespace
} // end namespace
#endif
