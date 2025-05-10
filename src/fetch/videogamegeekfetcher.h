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

#ifndef TELLICO_VIDEOGAMEGEEKFETCHER_H
#define TELLICO_VIDEOGAMEGEEKFETCHER_H

#include "xmlfetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

class VideoGameGeekFetcherTest;
namespace Tellico {
  namespace GUI {
    class ComboBox;
  }

  namespace Fetch {

/**
 * A fetcher for videogamegeek.com
 *
 * @author Robby Stephenson
 */
class VideoGameGeekFetcher : public XMLFetcher {
Q_OBJECT

friend class ::VideoGameGeekFetcherTest;

public:
  /**
   */
  VideoGameGeekFetcher(QObject* parent);
  /**
   */
  virtual ~VideoGameGeekFetcher();

  /**
   */
  virtual QString source() const override;
  virtual QString attribution() const override;
  virtual bool canSearch(FetchKey k) const override;
  virtual Type type() const override { return VideoGameGeek; }
  virtual bool canFetch(int type) const override;

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const VideoGameGeekFetcher* fetcher = nullptr);
    virtual void saveConfigHook(KConfigGroup&) override;
    virtual QString preferredName() const override;
  private:
     GUI::ComboBox* m_imageCombo;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private:
  virtual void readConfigHook(const KConfigGroup& cg) override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  virtual void resetSearch() override {}
  virtual QUrl searchUrl() override;
  virtual void parseData(QByteArray&) override;
  virtual Data::EntryPtr fetchEntryHookData(Data::EntryPtr entry) override;

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
