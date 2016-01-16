/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
    Copyright (C) 2005-2009 Steve Beattie <sbeattie@suse.de>
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

#include "boardgamecollection.h"

#include <KLocalizedString>

namespace {
  static const char* boardgame_general = I18N_NOOP("General");
  static const char* boardgame_personal = I18N_NOOP("Personal");
}

using Tellico::Data::BoardGameCollection;

BoardGameCollection::BoardGameCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Board Games") : title_) {
  setDefaultGroupField(QLatin1String("genre"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList BoardGameCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  list.append(createDefaultField(TitleField));

  field = new Field(QLatin1String("genre"), i18n("Genre"));
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("mechanism"), i18n("Mechanism"));
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("year"), i18n("Release Year"), Field::Number);
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("publisher"), i18n("Publisher"));
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("designer"), i18n("Designer"));
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("num-player"), i18n("Number of Players"), Field::Number);
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("playing-time"), i18n("Playing Time"), Field::Number);
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("minimum-age"), i18n("Minimum Age"), Field::Number);
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("description"), i18n("Description"), Field::Para);
  list.append(field);

  field = new Field(QLatin1String("rating"), i18n("Rating"), Field::Rating);
  field->setCategory(i18n(boardgame_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(boardgame_personal));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QLatin1String("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(boardgame_personal));
  list.append(field);

  field = new Field(QLatin1String("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(boardgame_personal));
  list.append(field);

  field = new Field(QLatin1String("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(boardgame_personal));
  list.append(field);

  field = new Field(QLatin1String("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QLatin1String("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(boardgame_personal));
  list.append(field);

  list.append(createDefaultField(IDField));
  list.append(createDefaultField(CreatedDateField));
  list.append(createDefaultField(ModifiedDateField));

  return list;
}
