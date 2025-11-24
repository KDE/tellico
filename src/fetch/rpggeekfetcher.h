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

#include "abstractbggfetcher.h"

namespace Tellico {
  namespace Fetch {

/**
 * A fetcher for rpggeek.com
 *
 * @author Robby Stephenson
 */
class RPGGeekFetcher : public AbstractBGGFetcher {
Q_OBJECT

public:
  RPGGeekFetcher(QObject* parent);

  virtual Type type() const override { return RPGGeek; }
  virtual bool canFetch(int type) const override;

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  class ConfigWidget : public AbstractBGGFetcher::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const RPGGeekFetcher* fetcher = nullptr);
    virtual QString preferredName() const override;
  };

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private:
  virtual QString bggType() const override;
};

  } // end namespace
} // end namespace
#endif
