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
#include "../core/tellico_strings.h"

#include <KLocalizedString>

using Tellico::Data::CardCollection;

CardCollection::CardCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Cards") : title_) {
  setDefaultGroupField(QStringLiteral("series"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList CardCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = Field::createDefaultField(Field::TitleField);
  field->setProperty(QStringLiteral("template"), QStringLiteral("%{year} %{brand} %{player}"));
  field->setFlags(Field::NoDelete | Field::Derived);
  field->setFormatType(FieldFormat::FormatNone);
  list.append(field);

  field = new Field(QStringLiteral("player"), i18n("Player"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("team"), i18n("Team"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QStringLiteral("brand"), i18n("Brand"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  // might not be totally numeric!
  field = new Field(QStringLiteral("number"), i18n("Card Number"));
  field->setCategory(TC_I18N1(categoryGeneral));
  list.append(field);

  field = new Field(QStringLiteral("year"), i18n("Year"), Field::Number);
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("series"), i18n("Series"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QStringLiteral("type"), i18n("Card Type"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("pur_date"), i18n("Purchase Date"));
  field->setCategory(TC_I18N1(categoryPersonal));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QStringLiteral("pur_price"), i18n("Purchase Price"));
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("location"), i18n("Location"));
  field->setCategory(TC_I18N1(categoryPersonal));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("keyword"), i18n("Keywords"));
  field->setCategory(TC_I18N1(categoryPersonal));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("quantity"), i18n("Quantity"), Field::Number);
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("front"), i18n("Front Image"), Field::Image);
  list.append(field);

  field = new Field(QStringLiteral("back"), i18n("Back Image"), Field::Image);
  list.append(field);

  field = new Field(QStringLiteral("comments"), i18n("Comments"), Field::Para);
  list.append(field);

  list.append(Field::createDefaultField(Field::IDField));
  list.append(Field::createDefaultField(Field::CreatedDateField));
  list.append(Field::createDefaultField(Field::ModifiedDateField));

  return list;
}
