/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_GOOGLESCHOLARFETCHER_H
#define TELLICO_GOOGLESCHOLARFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>
#include <QRegExp>

class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for Google Scholar
 *
 * @author Robby Stephenson
 */
class GoogleScholarFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  GoogleScholarFetcher(QObject* parent);
  /**
   */
  virtual ~GoogleScholarFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void continueSearch();
  // amazon can search title or person
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person || k == Keyword; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntryHook(uint uid);
  virtual Type type() const { return GoogleScholar; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const GoogleScholarFetcher* fetcher = 0);
    virtual void saveConfigHook(KConfigGroup&) {}
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields() { return StringHash(); }

private slots:
  void slotComplete(KJob* job);

private:
  virtual void search();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);
  void doSearch();

  int m_limit;
  int m_start;
  int m_total;

  QHash<int, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;

  QRegExp m_bibtexRx;
  bool m_cookieIsSet;
};

  } // end namespace
} // end namespace
#endif
