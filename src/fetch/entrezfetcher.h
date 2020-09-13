/***************************************************************************
    Copyright (C) 2005-2020 Robby Stephenson <robby@periapsis.org>
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
#include <QElapsedTimer>

class QLineEdit;

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

  virtual QString source() const Q_DECL_OVERRIDE;
  virtual bool isSearching() const Q_DECL_OVERRIDE { return m_started; }
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE;
  virtual void continueSearch() Q_DECL_OVERRIDE;
  virtual void stop() Q_DECL_OVERRIDE;
  virtual Data::EntryPtr fetchEntryHook(uint uid) Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return Entrez; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;
  virtual void readConfigHook(const KConfigGroup& config) Q_DECL_OVERRIDE;
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const Q_DECL_OVERRIDE;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const EntrezFetcher* fetcher=nullptr);
    virtual void saveConfigHook(KConfigGroup&) Q_DECL_OVERRIDE;
    virtual QString preferredName() const Q_DECL_OVERRIDE;
  private:
    QLineEdit* m_apiKeyEdit;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  virtual void search() Q_DECL_OVERRIDE;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) Q_DECL_OVERRIDE;
  void initXSLTHandler();
  void doSummary();

  void searchResults(const QByteArray& data);
  void summaryResults(const QByteArray& data);
  // honor throttle limit for the API
  void markTime();

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

  QString m_apiKey;

  QHash<uint, Data::EntryPtr> m_entries; // map from search result id to entry
  QHash<uint, int> m_matches; // search result id to pubmed id
  QPointer<KIO::StoredTransferJob> m_job;
  QElapsedTimer m_idleTime;

  QString m_queryKey;
  QString m_webEnv;
  Step m_step;

  bool m_started;
};

  } // end namespace
} // end namespace

#endif
