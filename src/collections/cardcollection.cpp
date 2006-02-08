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
   : Collection(title_, i18n("Cards")) {
  setTitle(title_.isNull() ? i18n("My Cards") : title_);
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("series"));
}

Tellico::Data::FieldVec CardCollection::defaultFields() {
  FieldVec list;
  FieldPtr field;

  field = new Field(QString::fromLatin1("title"), i18n("Title"), Field::Dependent);
  field->setCategory(i18n(card_general));
  field->setDescription(QString::fromLatin1("%{year} %{brand} %{player}"));
  field->setFlags(Field::NoDelete);
  list.append(field);

  field = new Field(QString::fromLatin1("player"), i18n("Player"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QString::fromLatin1("team"), i18n("Team"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("brand"), i18n("Brand"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  // might not be totally numeric!
  field = new Field(QString::fromLatin1("number"), i18n("Card Number"));
  field->setCategory(i18n(card_general));
  list.append(field);

  field = new Field(QString::fromLatin1("year"), i18n("Year"), Field::Number);
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("series"), i18n("Series"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("type"), i18n("Card Type"));
  field->setCategory(i18n(card_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(card_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(card_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("location"), i18n("Location"));
  field->setCategory(i18n(card_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(card_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("keyword"), i18n("Keywords"));
  field->setCategory(i18n(card_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("quantity"), i18n("Quantity"), Field::Number);
  field->setCategory(i18n(card_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("front"), i18n("Front Image"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("back"), i18n("Back Image"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("comments"), i18n("Comments"), Field::Para);
  list.append(field);

  return list;
}

#include "cardcollection.moc"
