/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>

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

#ifndef GAMECOLLECTION_H
#define GAMECOLLECTION_H

#include "../collection.h"

namespace Tellico {
  namespace Data {

/**
 * A collection for games.
 */
class GameCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param title The title of the collection
   */
  explicit GameCollection(bool addDefaultFields, const QString& title = QString());

  virtual Type type() const override { return Game; }

  static FieldList defaultFields();

  enum GamePlatform {
      UnknownPlatform = 0,
      Linux,
      MacOS,
      Windows,
      iOS,
      Android,
      Xbox,
      Xbox360,
      XboxOne,
      XboxSeriesX,
      PlayStation,
      PlayStation2,
      PlayStation3,
      PlayStation4,
      PlayStation5,
      PlayStationPortable,
      PlayStationVita,
      GameBoy,
      GameBoyColor,
      GameBoyAdvance,
      Nintendo,
      SuperNintendo,
      Nintendo64,
      NintendoGameCube,
      NintendoWii,
      NintendoWiiU,
      NintendoSwitch,
      NintendoDS,
      Nintendo3DS,
      Genesis,
      Dreamcast,
      LastPlatform
  };
  static QString normalizePlatform(const QString& platformName);
  static GamePlatform guessPlatform(const QString& platformName);
  static QString platformName(GamePlatform platform);

  enum EsrbRating {
    UnknownEsrb = 0,
    Unrated,
    Adults,
    Mature,
    Teen,
    Everyone10,
    Everyone,
    EarlyChildhood,
    Pending
  };
  static QStringList esrbRatings();
  static QString esrbRating(EsrbRating rating);
};

  } // end namespace
} // end namespace
#endif
