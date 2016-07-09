/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ENTREZFETCHER_H
#define TELLICO_ENTREZFETCHER_H

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
class EntrezFetcher : public Fetcher {
Q_OBJECT

public:
  EntrezFetcher(QObject* parent);
  /**
   */
  virtual ~EntrezFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual bool canSearch(FetchKey k) const;
  virtual void continueSearch();
  virtual void stop();
  virtual Data::EntryPtr fetchEntryHook(uint uid);
  virtual Type type() const { return Entrez; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const EntrezFetcher* fetcher=0);
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  virtual void search();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);
  void initXSLTHandler();
  void doSummary();

  void searchResults(const QByteArray& data);
  void summaryResults(const QByteArray& data);

  enum Step {
    Begin,
    Search,
    Summary,
    Fetch
  };

  XSLTHandler* m_xsltHandler;
  QString m_dbname;

  int m_start;
  int m_total;

  QHash<int, Data::EntryPtr> m_entries; // map from search result id to entry
  QHash<int, int> m_matches; // search result id to pubmed id
  QPointer<KIO::StoredTransferJob> m_job;

  QString m_queryKey;
  QString m_webEnv;
  Step m_step;

  bool m_started;
};

  } // end namespace
} // end namespace

#endif
