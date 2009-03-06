/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "winecollection.h"

#include <klocale.h>

namespace {
  static const char* wine_general = I18N_NOOP("General");
  static const char* wine_personal = I18N_NOOP("Personal");
}

using Tellico::Data::WineCollection;

WineCollection::WineCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_.isEmpty() ? i18n("My Wines") : title_) {
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QLatin1String("type"));
}

Tellico::Data::FieldList WineCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = new Field(QLatin1String("title"), i18n("Title"), Field::Dependent);
  field->setCategory(i18n(wine_general));
  field->setDescription(QLatin1String("%{vintage} %{producer} %{varietal}"));
  field->setFlags(Field::NoDelete);
  list.append(field);

  field = new Field(QLatin1String("producer"), i18nc("Wine Producer", "Producer"));
  field->setCategory(i18n(wine_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("appellation"), i18n("Appellation"));
  field->setCategory(i18n(wine_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("varietal"), i18n("Varietal"));
  field->setCategory(i18n(wine_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("vintage"), i18n("Vintage"), Field::Number);
  field->setCategory(i18n(wine_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  QStringList type;
  type << i18n("Red Wine") << i18n("White Wine") << i18n("Sparkling Wine");
  field = new Field(QLatin1String("type"), i18n("Type"), type);
  field->setCategory(i18n(wine_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("country"), i18n("Country"));
  field->setCategory(i18n(wine_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(wine_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QLatin1String("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(wine_personal));
  list.append(field);

  field = new Field(QLatin1String("location"), i18n("Location"));
  field->setCategory(i18n(wine_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("quantity"), i18n("Quantity"), Field::Number);
  field->setCategory(i18n(wine_personal));
  list.append(field);

  field = new Field(QLatin1String("drink-by"), i18n("Drink By"), Field::Number);
  field->setCategory(i18n(wine_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("rating"), i18n("Rating"), Field::Rating);
  field->setCategory(i18n(wine_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(wine_personal));
  list.append(field);

  field = new Field(QLatin1String("label"), i18n("Label Image"), Field::Image);
  list.append(field);

  field = new Field(QLatin1String("comments"), i18n("Comments"), Field::Para);
  list.append(field);

  return list;
}

#include "winecollection.moc"
