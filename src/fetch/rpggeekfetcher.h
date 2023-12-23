/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_RPGGEEKFETCHER_H
#define TELLICO_RPGGEEKFETCHER_H

#include "xmlfetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

class RPGGeekFetcherTest;
namespace Tellico {
  namespace GUI {
    class ComboBox;
  }

  namespace Fetch {

/**
 * A fetcher for rpggeek.com
 *
 * @author Robby Stephenson
 */
class RPGGeekFetcher : public XMLFetcher {
Q_OBJECT

friend class ::RPGGeekFetcherTest;

public:
  /**
   */
  RPGGeekFetcher(QObject* parent);
  /**
   */
  virtual ~RPGGeekFetcher();

  /**
   */
  virtual QString source() const Q_DECL_OVERRIDE;
  virtual QString attribution() const Q_DECL_OVERRIDE;
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return RPGGeek; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const Q_DECL_OVERRIDE;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const RPGGeekFetcher* fetcher = nullptr);
    virtual void saveConfigHook(KConfigGroup&) Q_DECL_OVERRIDE;
    virtual QString preferredName() const Q_DECL_OVERRIDE;
  private:
    GUI::ComboBox* m_imageCombo;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private:
  virtual void readConfigHook(const KConfigGroup& cg) Q_DECL_OVERRIDE;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) Q_DECL_OVERRIDE;
  virtual void resetSearch() Q_DECL_OVERRIDE {}
  virtual QUrl searchUrl() Q_DECL_OVERRIDE;
  virtual void parseData(QByteArray&) Q_DECL_OVERRIDE {}
  virtual Data::EntryPtr fetchEntryHookData(Data::EntryPtr entry) Q_DECL_OVERRIDE;

  enum ImageSize {
    NoImage=0,
    SmallImage=1, // small is really the thumb size
    LargeImage=2
  };
  ImageSize m_imageSize;
};

  } // end namespace
} // end namespace
#endif
