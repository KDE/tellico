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

#include "gamecollection.h"
#include "../tellico_debug.h"
#include "../core/tellico_strings.h"

#include <KLocalizedString>

using Tellico::Data::GameCollection;

GameCollection::GameCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Video Games") : title_) {
  setDefaultGroupField(QStringLiteral("platform"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList GameCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  list.append(Field::createDefaultField(Field::TitleField));

  QStringList platform;
  platform << platformName(XboxSeriesX) << platformName(XboxOne) << platformName(Xbox360) << platformName(Xbox)
           << platformName(PlayStation5) << platformName(PlayStation4) << platformName(PlayStation3) << platformName(PlayStation2) << platformName(PlayStation)
           << platformName(PlayStationPortable) << platformName(PlayStationVita)
           << platformName(NintendoSwitch) << platformName(NintendoWiiU)
           << platformName(NintendoWii)  << platformName(Nintendo3DS) << platformName(NintendoDS)
           << platformName(Nintendo64)  << platformName(SuperNintendo) << platformName(Nintendo)
           << platformName(NintendoGameCube) << platformName(Dreamcast)
           << platformName(Windows) << platformName(MacOS) << platformName(Linux);
  field = new Field(QStringLiteral("platform"), i18n("Platform"), platform);
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("genre"), i18n("Genre"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("year"), i18n("Release Year"), Field::Number);
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("publisher"), i18nc("Games - Publisher", "Publisher"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("developer"), i18n("Developer"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("certification"), i18n("ESRB Rating"), esrbRatings());
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("description"), i18n("Description"), Field::Para);
  list.append(field);

  field = new Field(QStringLiteral("rating"), i18n("Personal Rating"), Field::Rating);
  field->setCategory(TC_I18N1(categoryPersonal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("completed"), i18n("Completed"), Field::Bool);
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("pur_date"), i18n("Purchase Date"));
  field->setCategory(TC_I18N1(categoryPersonal));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QStringLiteral("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("pur_price"), i18n("Purchase Price"));
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QStringLiteral("comments"), i18n("Comments"), Field::Para);
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  list.append(Field::createDefaultField(Field::IDField));
  list.append(Field::createDefaultField(Field::CreatedDateField));
  list.append(Field::createDefaultField(Field::ModifiedDateField));

  return list;
}


QString GameCollection::normalizePlatform(const QString& platformName_) {
  if(platformName_.isEmpty()) return QString();
  GamePlatform platform = guessPlatform(platformName_);
  if(platform == UnknownPlatform) {
    QString platformName = platformName_;
    if(platformName.startsWith(QStringLiteral("Microsoft"))) {
      platformName = platformName.mid(sizeof("Microsoft"));
    } else if(platformName.startsWith(QStringLiteral("Sony Playstation"))) {
      // default video game collection has no space between 'PlayStation' and #
      platformName = QStringLiteral("PlayStation") + platformName.mid(sizeof("Sony Playstation"));
    }
    myDebug() << "No default name for" << platformName_ << "; return" << platformName;
    return platformName;
  }
  return platformName(platform);
}

Tellico::Data::GameCollection::GamePlatform GameCollection::guessPlatform(const QString& name_) {
  // try to be smart about guessing the platform from its name
  if(name_.contains(QStringLiteral("PlayStation"), Qt::CaseInsensitive)) {
    if(name_.contains(QStringLiteral("Vita"), Qt::CaseInsensitive)) {
      return PlayStationVita;
    } else if(name_.contains(QStringLiteral("Portable"), Qt::CaseInsensitive)) {
      return PlayStationPortable;
    } else if(name_.contains(QStringLiteral("5"))) {
      return PlayStation5;
    } else if(name_.contains(QStringLiteral("4"))) {
      return PlayStation4;
    } else if(name_.contains(QStringLiteral("3"))) {
      return PlayStation3;
    } else if(name_.contains(QStringLiteral("2"))) {
      return PlayStation2;
    } else {
      return PlayStation;
    }
  } else if(name_.contains(QStringLiteral("PSP"), Qt::CaseInsensitive)) {
    return PlayStationPortable;
  } else if(name_.contains(QStringLiteral("Xbox"), Qt::CaseInsensitive)) {
    if(name_.contains(QStringLiteral("One"), Qt::CaseInsensitive)) {
      return XboxOne;
    } else if(name_.contains(QStringLiteral("360"))) {
      return Xbox360;
    } else if(name_.endsWith(QStringLiteral("X"))) {
      return XboxSeriesX;
    } else {
      return Xbox;
    }
  } else if(name_.contains(QStringLiteral("Switch"), Qt::CaseInsensitive)) {
    return NintendoSwitch;
  } else if(name_.contains(QStringLiteral("Wii"), Qt::CaseInsensitive)) {
    if(name_.contains(QStringLiteral("U"), Qt::CaseInsensitive)) {
      return NintendoWiiU;
    } else {
      return NintendoWii;
    }
  } else if(name_.contains(QStringLiteral("PC"), Qt::CaseInsensitive) ||
            name_.contains(QStringLiteral("Windows"), Qt::CaseInsensitive)) {
    return Windows;
  } else if(name_.contains(QStringLiteral("Mac"), Qt::CaseInsensitive)) {
    return MacOS;
  } else if(name_.contains(QStringLiteral("3DS"), Qt::CaseInsensitive)) {
    return Nintendo3DS;
  } else if(name_.contains(QStringLiteral("DS"), Qt::CaseInsensitive)) {
    return NintendoDS;
  } else if(name_ == QStringLiteral("Nintendo 64")) {
    return Nintendo64;
  } else if(name_.contains(QStringLiteral("GameCube"), Qt::CaseInsensitive)) {
    return NintendoGameCube;
  } else if(name_.contains(QStringLiteral("Advance"), Qt::CaseInsensitive)) {
    return GameBoyAdvance;
  } else if(name_.contains(QStringLiteral("Game Boy Color"), Qt::CaseInsensitive) ||
            name_.contains(QStringLiteral("GameBoy Color"), Qt::CaseInsensitive)) {
    return GameBoyColor;
  } else if(name_.contains(QStringLiteral("Game Boy"), Qt::CaseInsensitive) ||
            name_.contains(QStringLiteral("GameBoy"), Qt::CaseInsensitive)) {
    return GameBoy;
  } else if(name_.contains(QStringLiteral("SNES"), Qt::CaseInsensitive) ||
            name_.contains(QStringLiteral("Super Nintendo"), Qt::CaseInsensitive)) {
    return SuperNintendo;
    // only return Nintendo if equal or includes Original or Entertainment
    // could be platforms like "Nintendo Virtual Boy"
  } else if(name_ == QLatin1String("Nintendo") ||
            name_ == QLatin1String("NES") ||
            name_.contains(QStringLiteral("Nintendo Entertainment"), Qt::CaseInsensitive)) {
    return Nintendo;
  } else if(name_.contains(QStringLiteral("Genesis"), Qt::CaseInsensitive)) {
    return Genesis;
  } else if(name_.contains(QStringLiteral("Dreamcast"), Qt::CaseInsensitive)) {
    return Dreamcast;
  } else if(name_.contains(QStringLiteral("Linux"), Qt::CaseInsensitive)) {
    return Linux;
  } else if(name_.contains(QStringLiteral("ios"), Qt::CaseInsensitive)) {
    return iOS;
  } else if(name_.contains(QStringLiteral("Android"), Qt::CaseInsensitive)) {
    return Android;
  }
//  myDebug() << "No platform guess for" << name_;
  return UnknownPlatform;
}

QString GameCollection::platformName(GamePlatform platform_) {
  switch(platform_) {
    case Linux:               return i18n("Linux");
    case MacOS:               return i18n("Mac OS");
    case Windows:             return i18nc("Windows Platform", "Windows");
    case iOS:                 return i18nc("iOS Platform", "iOS");
    case Android:             return i18nc("Android Platform", "Android");
    case Xbox:                return i18n("Xbox");
    case Xbox360:             return i18n("Xbox 360");
    case XboxOne:             return i18n("Xbox One");
    case XboxSeriesX:         return i18n("Xbox Series X");
    case PlayStation:         return i18n("PlayStation");
    case PlayStation2:        return i18n("PlayStation2");
    case PlayStation3:        return i18n("PlayStation3");
    case PlayStation4:        return i18n("PlayStation4");
    case PlayStation5:        return i18n("PlayStation5");
    case PlayStationPortable: return i18nc("PlayStation Portable", "PSP");
    case PlayStationVita:     return i18n("PlayStation Vita");
    case GameBoy:             return i18n("Game Boy");
    case GameBoyColor:        return i18n("Game Boy Color");
    case GameBoyAdvance:      return i18n("Game Boy Advance");
    case Nintendo:            return i18n("Nintendo");
    case SuperNintendo:       return i18n("Super Nintendo");
    case Nintendo64:          return i18n("Nintendo 64");
    case NintendoGameCube:    return i18n("GameCube");
    case NintendoWii:         return i18n("Nintendo Wii");
    case NintendoWiiU:        return i18n("Nintendo WiiU");
    case NintendoSwitch:      return i18n("Nintendo Switch");
    case NintendoDS:          return i18n("Nintendo DS");
    case Nintendo3DS:         return i18n("Nintendo 3DS");
    case Genesis:             return i18nc("Sega Genesis", "Genesis");
    case Dreamcast:           return i18n("Dreamcast");
    case UnknownPlatform:     break;
    case LastPlatform:        break;
  }
  myDebug() << "Failed to return platform name for" << platform_;
  return QString();
}

QStringList GameCollection::esrbRatings() {
  static const QRegularExpression rx(QLatin1String("\\s*,\\s*"));
  /* TRANSLATORS: There must be 8 translated phrases, matching the ESRB system */
  QStringList cert = i18nc("Video game ratings - must be 8 phrases matching ESRB system - "
                           "Unrated, Adults Only, Mature, Teen, Everyone 10+, Everyone, Early Childhood, Pending",
                           "Unrated, Adults Only, Mature, Teen, Everyone 10+, Everyone, Early Childhood, Pending")
                     .split(rx, Qt::SkipEmptyParts);
  return cert;
}

QString GameCollection::esrbRating(EsrbRating rating_) {
  static const QStringList ratings = esrbRatings();
  Q_ASSERT(ratings.size() == 8);
  if(ratings.size() < 8) {
    myWarning() << "ESRB rating list is badly translated, return empty string";
    return QString();
  }
  switch(rating_) {
    case Unrated:        return ratings.at(0);
    case Adults:         return ratings.at(1);
    case Mature:         return ratings.at(2);
    case Teen:           return ratings.at(3);
    case Everyone10:     return ratings.at(4);
    case Everyone:       return ratings.at(5);
    case EarlyChildhood: return ratings.at(6);
    case Pending:        return ratings.at(7);
    default:
      return QString();
  }
}
