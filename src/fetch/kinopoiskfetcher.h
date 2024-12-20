/***************************************************************************
    Copyright (C) 2017-2021 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_KINOPOISKFETCHER_H
#define TELLICO_FETCH_KINOPOISKFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>
#include <QJsonValue>
#include <QJsonObject>

class QUrl;
class QSpinBox;

class KJob;
namespace KIO {
  class Job;
  class StoredTransferJob;
}

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for www.kinopoisk.ru
 *
 * @author Robby Stephenson
 */
class KinoPoiskFetcher : public Fetcher {
Q_OBJECT

public:
  KinoPoiskFetcher(QObject* parent);
  virtual ~KinoPoiskFetcher();

  virtual QString source() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual bool canSearch(FetchKey k) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return KinoPoisk; }
  virtual bool canFetch(int type) const override;
  virtual void readConfigHook(const KConfigGroup& config) override;

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const KinoPoiskFetcher* fetcher = nullptr);
    virtual void saveConfigHook(KConfigGroup&) override;
    virtual QString preferredName() const override;
  private:
    QSpinBox* m_numCast;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);
  void slotRedirection(KIO::Job* job, const QUrl& toUrl);

private:
  static QString fieldNameFromKey(const QString& key);
  static QString fieldValueFromObject(const QJsonObject& obj, const QString& field,
                                      const QJsonValue& value, const QStringList& allowed);
  static QString mpaaRating(const QString& value, const QStringList& allowed);

  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  Data::EntryPtr requestEntry(const QString& filmId);
  Data::EntryPtr parseEntry(const QString& str);
  Data::EntryPtr parseEntryLinkedData(const QString& str);

  QHash<uint, Data::EntryPtr> m_entries;
  QHash<uint, QUrl> m_matches;
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
  bool m_redirected;
  QUrl m_redirectUrl;
  QString m_apiKey;
  int m_numCast;
};

  } // end namespace
} // end namespace
#endif
