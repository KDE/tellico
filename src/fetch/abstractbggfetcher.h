/***************************************************************************
    Copyright (C) 2014-2025 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ABSTRACTBGGFETCHER_H
#define TELLICO_ABSTRACTBGGFETCHER_H

#include "xmlfetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

class QLineEdit;

class BoardGameGeekFetcherTest;
class RPGGeekFetcherTest;
class VideoGameGeekFetcherTest;

namespace Tellico {
  namespace GUI {
    class ComboBox;
  }

  namespace Fetch {

/**
 * A fetcher for boardgamegeek.com data sources
 *
 * @author Robby Stephenson
 */
class AbstractBGGFetcher : public XMLFetcher {
Q_OBJECT

friend class ::BoardGameGeekFetcherTest;
friend class ::RPGGeekFetcherTest;
friend class ::VideoGameGeekFetcherTest;

public:
  /**
   */
  AbstractBGGFetcher(QObject* parent);
  /**
   */
  virtual ~AbstractBGGFetcher();

  /**
   */
  virtual QString source() const override;
  virtual QString attribution() const override;
  virtual bool canSearch(FetchKey k) const override;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const AbstractBGGFetcher* fetcher = nullptr);
    virtual void saveConfigHook(KConfigGroup&) override;
  private:
    QLineEdit* m_apiKeyEdit;
    GUI::ComboBox* m_imageCombo;
  };
  friend class ConfigWidget;

private:
  virtual QUrl searchUrl() override;
  virtual void doSearchHook(KIO::Job* job) override;
  virtual void readConfigHook(const KConfigGroup& cg) override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  virtual void resetSearch() override {}
  virtual void parseData(QByteArray& data) override;
  virtual Data::EntryPtr fetchEntryHookData(Data::EntryPtr entry) override;
  virtual QString bggType() const = 0;

  enum ImageSize {
    NoImage=0,
    SmallImage=1, // small is really the thumb size
    LargeImage=2
  };
  ImageSize m_imageSize;
  QString m_apiKey;
};

  } // end namespace
} // end namespace
#endif
