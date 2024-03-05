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

#include "winecollection.h"
#include "../core/tellico_strings.h"

#include <KLocalizedString>

using Tellico::Data::WineCollection;

WineCollection::WineCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Wines") : title_) {
  setDefaultGroupField(QStringLiteral("type"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList WineCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = Field::createDefaultField(Field::TitleField);
  field->setProperty(QStringLiteral("template"), QStringLiteral("%{vintage} %{producer:1} %{varietal:1}"));
  field->setFlags(Field::NoDelete | Field::Derived);
  field->setFormatType(FieldFormat::FormatNone);
  list.append(field);

  field = new Field(QStringLiteral("producer"), i18nc("Wine Producer", "Producer"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("appellation"), i18n("Appellation"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("varietal"), i18n("Varietal"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("vintage"), i18n("Vintage"), Field::Number);
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  QStringList type;
  type << i18n("Red Wine") << i18n("White Wine") << i18n("Sparkling Wine");
  field = new Field(QStringLiteral("type"), i18n("Type"), type);
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("country"), i18n("Country"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("pur_date"), i18n("Purchase Date"));
  field->setCategory(TC_I18N(categoryPersonal));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QStringLiteral("pur_price"), i18n("Purchase Price"));
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("location"), i18n("Location"));
  field->setCategory(TC_I18N(categoryPersonal));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("quantity"), i18n("Quantity"), Field::Number);
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("drink-by"), i18n("Drink By"), Field::Number);
  field->setCategory(TC_I18N(categoryPersonal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("rating"), i18n("Rating"), Field::Rating);
  field->setCategory(TC_I18N(categoryPersonal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("label"), i18n("Label Image"), Field::Image);
  list.append(field);

  field = new Field(QStringLiteral("description"), i18n("Description"), Field::Para);
  list.append(field);

  field = new Field(QStringLiteral("comments"), i18n("Comments"), Field::Para);
  list.append(field);

  list.append(Field::createDefaultField(Field::IDField));
  list.append(Field::createDefaultField(Field::CreatedDateField));
  list.append(Field::createDefaultField(Field::ModifiedDateField));

  return list;
}
