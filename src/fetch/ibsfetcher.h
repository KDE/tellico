/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_IBSFETCHER_H
#define TELLICO_FETCH_IBSFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>

class KUrl;
class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for animenfo.com
 *
 * @author Robby Stephenson
 */
class IBSFetcher : public Fetcher {
Q_OBJECT

public:
  IBSFetcher(QObject* parent);
  virtual ~IBSFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  // can search title, person, isbn, or keyword. No UPC or Raw for now.
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person || k == ISBN || k == Keyword; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return IBS; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_);
    virtual void saveConfig(KConfigGroup&) {}
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash optionalFields() { return StringHash(); }

private slots:
  void slotComplete(KJob* job);
  void slotCompleteISBN(KJob* job);

private:
  virtual void search();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);
  Data::EntryPtr parseEntry(const QString& str);

  int m_total;
  QHash<int, Data::EntryPtr> m_entries;
  QHash<int, KUrl> m_matches;
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
};

  } // end namespace
} // end namespace
#endif
