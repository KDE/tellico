/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_MOVIEMETERFETCHER_H
#define TELLICO_MOVIEMETERFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QEventLoop>

namespace KXmlRpc {
  class Client;
}

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class MovieMeterFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  MovieMeterFetcher(QObject* parent);
  /**
   */
  virtual ~MovieMeterFetcher();

  /**
   */
  virtual QString source() const;
  virtual QString attribution() const;
  virtual bool isSearching() const { return m_started; }
  virtual bool canSearch(FetchKey k) const;
  virtual void stop();
  virtual Data::EntryPtr fetchEntryHook(uint uid);
  virtual Type type() const { return MovieMeter; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const MovieMeterFetcher* fetcher = 0);
    virtual void saveConfigHook(KConfigGroup&);
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private slots:
  void gotSession(const QList<QVariant>&, const QVariant&);
  void gotFilmSearch(const QList<QVariant>&, const QVariant&);
  void gotFilmDetails(const QList<QVariant>&, const QVariant&);
  void gotFilmImage(const QList<QVariant>&, const QVariant&);
  void gotDirectorSearch(const QList<QVariant>&, const QVariant&);
  void gotDirectorFilms(const QList<QVariant>&, const QVariant&);
  void gotError(int code, const QString&, const QVariant&);

private:
  virtual void search();
  void checkSession();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);

  QHash<int, int> m_films;
  QHash<int, Data::EntryPtr> m_entries;

  bool m_started;
  KXmlRpc::Client* m_client;
  QString m_session;
  QDateTime m_validTill;
  QEventLoop m_loop;
  Data::CollPtr m_coll;
  Data::EntryPtr m_currEntry;
};

  } // end namespace
} // end namespace
#endif
