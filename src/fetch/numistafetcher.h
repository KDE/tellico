/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_NUMISTAFETCHER_H
#define TELLICO_NUMISTAFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QPointer>

class QLineEdit;

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
 * A fetcher for numista.com
 *
 * @author Robby Stephenson
 */
class NumistaFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  NumistaFetcher(QObject* parent);
  /**
   */
  virtual ~NumistaFetcher();

  /**
   */
  virtual QString source() const Q_DECL_OVERRIDE;
  virtual bool isSearching() const Q_DECL_OVERRIDE { return m_started; }
  virtual void continueSearch() Q_DECL_OVERRIDE;
  // amazon can search title or person
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE { return k == Keyword; }
  virtual void stop() Q_DECL_OVERRIDE;
  virtual Data::EntryPtr fetchEntryHook(uint uid) Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return Numista; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;
  virtual void readConfigHook(const KConfigGroup& config) Q_DECL_OVERRIDE;
  void setLimit(int limit);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
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
  void doSearch();
  Data::EntryPtr parseEntry(const QByteArray& data);

  int m_limit;
  int m_total;
  int m_page;

  QHash<uint, int> m_matches; // search result id to coin id
  QHash<uint, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;
  QString m_apiKey;
  QString m_locale;
  QString m_year;

  bool m_started;
};

class NumistaFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const NumistaFetcher* fetcher = nullptr);
  virtual void saveConfigHook(KConfigGroup&) Q_DECL_OVERRIDE;
  virtual QString preferredName() const Q_DECL_OVERRIDE;

private Q_SLOTS:
  void slotLangChanged();

private:
  QLineEdit* m_apiKeyEdit;
  GUI::ComboBox* m_langCombo;
};

  } // end namespace
} // end namespace
#endif