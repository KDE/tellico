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

#include "cardcollection.h"

#include <klocale.h>

namespace {
  static const char* card_general = I18N_NOOP("General");
  static const char* card_personal = I18N_NOOP("Personal");
}

using Tellico::Data::CardCollection;

CardCollection::CardCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_.isEmpty() ? i18n("My Cards") : title_) {
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QLatin1String("series"));
}

Tellico::Data::FieldList CardCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = new Field(QLatin1String("title"), i18n("Title"), Field::Dependent);
  field->setCategory(i18n(card_general));
  field->setDescription(QLatin1String("%{year} %{brand} %{player}"));
  field->setFlags(Field::NoDelete);
  list.append(field);

  field = new Field(QLatin1String("player"), i18n("Player"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QLatin1String("team"), i18n("Team"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QLatin1String("brand"), i18n("Brand"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
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
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QLatin1String("type"), i18n("Card Type"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(card_personal));
  field->setFormatFlag(Field::FormatDate);
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

  return list;
}

#include "cardcollection.moc"
