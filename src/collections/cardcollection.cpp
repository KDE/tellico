/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "cardcollection.h"

#include <klocale.h>

namespace {
  static const char* card_general = I18N_NOOP("General");
  static const char* card_personal = I18N_NOOP("Personal");
}

using Tellico::Data::CardCollection;

CardCollection::CardCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Cards") : title_) {
  setDefaultGroupField(QLatin1String("series"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList CardCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = createDefaultField(TitleField);
  field->setProperty(QLatin1String("template"), QLatin1String("%{year} %{brand} %{player}"));
  field->setFlags(Field::NoDelete | Field::Derived);
  field->setFormatType(FieldFormat::FormatNone);
  list.append(field);

  field = new Field(QLatin1String("player"), i18n("Player"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QLatin1String("team"), i18n("Team"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QLatin1String("brand"), i18n("Brand"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  // might not be totally numeric!
  field = new Field(QLatin1String("number"), i18n("Card Number"));
  field->setCategory(i18n(card_general));
  list.append(field);

  field = new Field(QLatin1String("year"), i18n("Year"), Field::Number);
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("series"), i18n("Series"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QLatin1String("type"), i18n("Card Type"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(card_personal));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QLatin1String("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(card_personal));
  list.append(field);

  field = new Field(QLatin1String("location"), i18n("Location"));
  field->setCategory(i18n(card_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(card_personal));
  list.append(field);

  field = new Field(QLatin1String("keyword"), i18n("Keywords"));
  field->setCategory(i18n(card_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("quantity"), i18n("Quantity"), Field::Number);
  field->setCategory(i18n(card_personal));
  list.append(field);

  field = new Field(QLatin1String("front"), i18n("Front Image"), Field::Image);
  list.append(field);

  field = new Field(QLatin1String("back"), i18n("Back Image"), Field::Image);
  list.append(field);

  field = new Field(QLatin1String("comments"), i18n("Comments"), Field::Para);
  list.append(field);

  list.append(createDefaultField(IDField));
  list.append(createDefaultField(CreatedDateField));
  list.append(createDefaultField(ModifiedDateField));

  return list;
}

#include "cardcollection.moc"
