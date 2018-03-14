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

#include <KLocalizedString>

namespace {
  static const char* game_general = I18N_NOOP("General");
  static const char* game_personal = I18N_NOOP("Personal");
}

using Tellico::Data::GameCollection;

GameCollection::GameCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Games") : title_) {
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
  platform << i18n("Xbox One") << i18n("Xbox 360") << i18n("Xbox")
           << i18n("PlayStation4") << i18n("PlayStation3") << i18n("PlayStation2") << i18n("PlayStation")
           << i18nc("PlayStation Portable", "PSP") << i18n("PlayStation Vita")
           << i18n("Nintendo Switch")
           << i18n("Nintendo Wii") << i18n("Nintendo 3DS") << i18n("Nintendo DS")
           << i18n("Nintendo 64") << i18n("Super Nintendo") << i18n("Nintendo")
           << i18n("GameCube") << i18n("Dreamcast") << i18nc("Sega Genesis", "Genesis")
           << i18n("Game Boy Advance") << i18n("Game Boy Color") << i18n("Game Boy")
           << i18nc("Windows Platform", "Windows") << i18n("Mac OS") << i18n("Linux");
  field = new Field(QStringLiteral("platform"), i18n("Platform"), platform);
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("genre"), i18n("Genre"));
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("year"), i18n("Release Year"), Field::Number);
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("publisher"), i18nc("Games - Publisher", "Publisher"));
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("developer"), i18n("Developer"));
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  QStringList cert = i18nc("Video game ratings - "
                           "Unrated, Adults Only, Mature, Teen, Everyone 10+, Everyone, Early Childhood, Pending",
                           "Unrated, Adults Only, Mature, Teen, Everyone 10+, Everyone, Early Childhood, Pending")
                     .split(QRegExp(QLatin1String("\\s*,\\s*")), QString::SkipEmptyParts);
  field = new Field(QStringLiteral("certification"), i18n("ESRB Rating"), cert);
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("description"), i18n("Description"), Field::Para);
  list.append(field);

  field = new Field(QStringLiteral("rating"), i18n("Personal Rating"), Field::Rating);
  field->setCategory(i18n(game_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("completed"), i18n("Completed"), Field::Bool);
  field->setCategory(i18n(game_personal));
  list.append(field);

  field = new Field(QStringLiteral("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(game_personal));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QStringLiteral("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(game_personal));
  list.append(field);

  field = new Field(QStringLiteral("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(game_personal));
  list.append(field);

  field = new Field(QStringLiteral("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(game_personal));
  list.append(field);

  field = new Field(QStringLiteral("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QStringLiteral("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(game_personal));
  list.append(field);

  list.append(Field::createDefaultField(Field::IDField));
  list.append(Field::createDefaultField(Field::CreatedDateField));
  list.append(Field::createDefaultField(Field::ModifiedDateField));

  return list;
}
