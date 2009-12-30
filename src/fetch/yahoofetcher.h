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

#ifndef YAHOOFETCHER_H
#define YAHOOFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>

class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  class XSLTHandler;
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class YahooFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  YahooFetcher(QObject* parent);
  /**
   */
  virtual ~YahooFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void continueSearch();
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntryHook(uint uid);
  virtual Type type() const { return Yahoo; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const YahooFetcher* fetcher = 0);
    virtual void saveConfigHook(KConfigGroup&) {}
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotComplete(KJob* job);

private:
  virtual void search();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);
  void initXSLTHandler();
  void doSearch();
  void getTracks(Data::EntryPtr entry);
  QString insertValue(const QString& str, const QString& value, int pos);

  XSLTHandler* m_xsltHandler;
  int m_limit;
  int m_start;
  int m_total;

  QHash<int, Data::EntryPtr> m_entries; // they get modified after collection is created, so can't be const
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
};

  } // end namespace
} // end namespace
#endif
