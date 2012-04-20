/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ABSTRACTALLOCINEFETCHER_H
#define TELLICO_ABSTRACTALLOCINEFETCHER_H

#include "xmlfetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

namespace Tellico {

  namespace Fetch {

/**
 * An abstract fetcher for the Allocine family of web sites
 *
 * @author Robby Stephenson
 */
class AbstractAllocineFetcher : public XMLFetcher {
Q_OBJECT

public:
  /**
   */
  AbstractAllocineFetcher(QObject* parent, const QString& baseUrl);
  /**
   */
  virtual ~AbstractAllocineFetcher();

  virtual bool canSearch(FetchKey k) const;
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const AbstractAllocineFetcher* fetcher = 0);
    virtual void saveConfigHook(KConfigGroup&);
    virtual QString preferredName() const;
  };
  friend class ConfigWidget;

private:
  virtual FetchRequest updateRequest(Data::EntryPtr entry);
  virtual void resetSearch();
  virtual KUrl searchUrl();
  virtual void parseData(QByteArray& data);
  virtual Data::EntryPtr fetchEntryHookData(Data::EntryPtr entry);

  QString m_apiKey;
  QString m_baseUrl;
};

/**
 * A fetcher for allocine.fr
 *
 * @author Robby Stephenson
 */
class AllocineFetcher : public AbstractAllocineFetcher {
Q_OBJECT

public:
  /**
   */
  AllocineFetcher(QObject* parent);

  virtual QString source() const;
  virtual Type type() const { return Allocine; }

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();
};

/**
 * A fetcher for screenrush.co.uk
 *
 * @author Robby Stephenson
 */
class ScreenRushFetcher : public AbstractAllocineFetcher {
Q_OBJECT

public:
  /**
   */
  ScreenRushFetcher(QObject* parent);

  virtual QString source() const;
  virtual Type type() const { return ScreenRush; }

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();
};

/**
 * A fetcher for filmstarts.de
 *
 * @author Robby Stephenson
 */
class FilmStartsFetcher : public AbstractAllocineFetcher {
Q_OBJECT

public:
  /**
   */
  FilmStartsFetcher(QObject* parent);

  virtual QString source() const;
  virtual Type type() const { return FilmStarts; }

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();
};

/**
 * A fetcher for sensacine.com
 *
 * @author Robby Stephenson
 */
class SensaCineFetcher : public AbstractAllocineFetcher {
Q_OBJECT

public:
  /**
   */
  SensaCineFetcher(QObject* parent);

  virtual QString source() const;
  virtual Type type() const { return SensaCine; }

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();
};

/**
 * A fetcher for beyazperde.com
 *
 * @author Robby Stephenson
 */
class BeyazperdeFetcher : public AbstractAllocineFetcher {
Q_OBJECT

public:
  /**
   */
  BeyazperdeFetcher(QObject* parent);

  virtual QString source() const;
  virtual Type type() const { return Beyazperde; }

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();
};

  } // end namespace
} // end namespace
#endif
