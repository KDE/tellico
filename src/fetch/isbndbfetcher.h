/***************************************************************************
    Copyright (C) 2006-2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_ISBNDBFETCHER_H
#define TELLICO_FETCH_ISBNDBFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>

class QLineEdit;
class QCheckBox;

class KJob;
namespace KIO {
  class StoredTransferJob;
}
class ISBNdbFetcherTest;

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class ISBNdbFetcher : public Fetcher {
Q_OBJECT

public:
  ISBNdbFetcher(QObject* parent = nullptr);
  ~ISBNdbFetcher();

  virtual QString source() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual void continueSearch() override;
  virtual bool canSearch(FetchKey k) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return ISBNdb; }
  virtual bool canFetch(int type) const override;
  virtual void readConfigHook(const KConfigGroup& config) override;

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;
  void setLimit(int limit_) { m_limit = limit_; }

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const ISBNdbFetcher* fetcher = nullptr);
    virtual void saveConfigHook(KConfigGroup&) override;
    virtual QString preferredName() const override;
  private:
    QLineEdit* m_apiKeyEdit;
    QCheckBox* m_enableBatchIsbn;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  friend class ::ISBNdbFetcherTest;
  void setTestUrl1(const QUrl& url) { m_testUrl1 = url; }

  virtual void search() override;
  void doSearch(const QString& term);
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  void populateEntry(Data::EntryPtr entry, const QJsonObject& obj);
  void endJob(KIO::StoredTransferJob* job);

  int m_limit;
  int m_total;
  int m_numResults;

  QHash<uint, Data::EntryPtr> m_entries;
  QList< QPointer<KIO::StoredTransferJob> > m_jobs;

  bool m_started;
  QString m_apiKey;
  bool m_batchIsbn;

  QUrl m_testUrl1;
};

  }
}
#endif
