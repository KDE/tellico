/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMDBFETCHER_H
#define TELLICO_IMDBFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QUrl>
#include <QPointer>

class QSpinBox;

class KJob;
namespace KIO {
  class Job;
  class StoredTransferJob;
}

class QCheckBox;
class QRegExpr;
class QRegularExpression;

class ImdbFetcherTest;

namespace Tellico {
  namespace GUI {
    class ComboBox;
  }
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class IMDBFetcher : public Fetcher {
Q_OBJECT

public:
  enum Lang {
    EN = 0,
    FR = 1,
    ES = 2,
    DE = 3,
    IT = 4,
    PT = 5
  };

  IMDBFetcher(QObject* parent);
  /**
   */
  virtual ~IMDBFetcher();

  virtual QString source() const Q_DECL_OVERRIDE;
  virtual bool isSearching() const Q_DECL_OVERRIDE { return m_started; }
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE;
  virtual void stop() Q_DECL_OVERRIDE;
  virtual Data::EntryPtr fetchEntryHook(uint uid) Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return IMDB; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;
  virtual void readConfigHook(const KConfigGroup& config) Q_DECL_OVERRIDE;

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const Q_DECL_OVERRIDE;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  friend class ::ImdbFetcherTest;
  static QString searchQuery();
  static QString titleQuery();
  static QString episodeQuery();

  virtual void search() Q_DECL_OVERRIDE;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) Q_DECL_OVERRIDE;

  void configureJob(QPointer<KIO::StoredTransferJob> job);
  Data::EntryPtr readGraphQL(const QString& imdbId, const QString& titleType);
  Data::EntryPtr parseResult(const QByteArray& data);

  QHash<uint, Data::EntryPtr> m_entries;
  QHash<uint, QString> m_matches;
  QHash<uint, QString> m_titleTypes;
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
  bool m_fetchImages;

  int m_numCast;
  QUrl m_url;
  Lang m_lang;
};

class IMDBFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const IMDBFetcher* fetcher = nullptr);

  virtual void saveConfigHook(KConfigGroup& config) Q_DECL_OVERRIDE;
  virtual QString preferredName() const Q_DECL_OVERRIDE;

private:
  QCheckBox* m_fetchImageCheck;
  QSpinBox* m_numCast;
};

  } // end namespace
} // end namespace

#endif
