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

#include "coincollection.h"

#include <klocale.h>

namespace {
  static const char* coin_general = I18N_NOOP("General");
  static const char* coin_personal = I18N_NOOP("Personal");
}

using Tellico::Data::CoinCollection;

CoinCollection::CoinCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_.isEmpty() ? i18n("My Coins") : title_) {
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("denomination"));
}

Tellico::Data::FieldList CoinCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = new Field(QString::fromLatin1("title"), i18n("Title"), Field::Dependent);
  field->setCategory(i18n(coin_general));
  // not i18n()
  field->setDescription(QString::fromLatin1("%{year}%{mintmark} %{type} %{denomination}"));
  field->setFlags(Field::NoDelete);
  list.append(field);

  field = new Field(QString::fromLatin1("type"), i18n("Type"));
  field->setCategory(i18n(coin_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  /* TRANSLATORS: denomination refers to the monetary value. */
  field = new Field(QString::fromLatin1("denomination"), i18n("Denomination"));
  field->setCategory(i18n(coin_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("year"), i18n("Year"), Field::Number);
  field->setCategory(i18n(coin_general));
  field->setFlags(Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("mintmark"), i18n("Mint Mark"));
  field->setCategory(i18n(coin_general));
  field->setFlags(Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("country"), i18n("Country"));
  field->setCategory(i18n(coin_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("set"), i18n("Coin Set"), Field::Bool);
  field->setCategory(i18n(coin_general));
  list.append(field);

  QStringList grade = i18nc("Coin grade levels - "
                            "Proof-65,Proof-60,Mint State-65,Mint State-60,"
                            "Almost Uncirculated-55,Almost Uncirculated-50,"
                            "Extremely Fine-40,Very Fine-30,Very Fine-20,Fine-12,"
                            "Very Good-8,Good-4,Fair",
                            "Proof-65,Proof-60,Mint State-65,Mint State-60,"
                            "Almost Uncirculated-55,Almost Uncirculated-50,"
                            "Extremely Fine-40,Very Fine-30,Very Fine-20,Fine-12,"
                            "Very Good-8,Good-4,Fair")
                      .split(QRegExp(QString::fromLatin1("\\s*,\\s*")), QString::SkipEmptyParts);
  field = new Field(QString::fromLatin1("grade"), i18n("Grade"), grade);
  field->setCategory(i18n(coin_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  QStringList service = i18nc("Coin grading services - "
                              "PCGS,NGC,ANACS,ICG,ASA,PCI",
                              "PCGS,NGC,ANACS,ICG,ASA,PCI")
                        .split(QRegExp(QString::fromLatin1("\\s*,\\s*")), QString::SkipEmptyParts);
  field = new Field(QString::fromLatin1("service"), i18n("Grading Service"), service);
  field->setCategory(i18n(coin_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(coin_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(coin_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("location"), i18n("Location"));
  field->setCategory(i18n(coin_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(coin_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("obverse"), i18n("Obverse"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("reverse"), i18n("Reverse"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(coin_personal));
  list.append(field);

  return list;
}

#include "coincollection.moc"
