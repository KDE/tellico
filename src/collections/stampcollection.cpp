/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "stampcollection.h"
#include "../collectionfactory.h"

#include <klocale.h>

namespace {
  static const char* stamp_general = I18N_NOOP("General");
  static const char* stamp_condition = I18N_NOOP("Condition");
  static const char* stamp_personal = I18N_NOOP("Personal");
}

using Tellico::Data::StampCollection;

StampCollection::StampCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_, CollectionFactory::entryName(Stamp), i18n("Stamps")) {
  setTitle(title_.isNull() ? i18n("My Stamps") : title_);
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("denomination"));
}

Tellico::Data::FieldList StampCollection::defaultFields() {
  FieldList list;
  Field* field;

  field = new Field(QString::fromLatin1("title"), i18n("Title"), Field::Dependent);
  field->setCategory(i18n(stamp_general));
  field->setDescription(QString::fromLatin1("%{year} %{description} %{denomination}"));
  field->setFlags(Field::NoDelete);
  list.append(field);

  field = new Field(QString::fromLatin1("description"), i18n("Description"));
  field->setCategory(i18n(stamp_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("denomination"), i18n("Denomination"));
  field->setCategory(i18n(stamp_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("country"), i18n("Country"));
  field->setCategory(i18n(stamp_general));
  field->setFormatFlag(Field::FormatPlain);
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("year"), i18n("Issue Year"), Field::Number);
  field->setCategory(i18n(stamp_general));
  field->setFlags(Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("color"), i18n("Color"));
  field->setCategory(i18n(stamp_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("scott"), i18n("Scott#"));
  field->setCategory(i18n(stamp_general));
  list.append(field);

  QStringList grade = QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                                         i18n("_:Stamp grade levels\n"
                                              "Superb,Extremely Fine,Very Fine,Fine,Average,Poor",
                                              "Superb,Extremely Fine,Very Fine,Fine,Average,Poor"),
                                         false);
  field = new Field(QString::fromLatin1("grade"), i18n("Grade"), grade);
  field->setCategory(i18n(stamp_condition));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("cancelled"), i18n("Cancelled"), Field::Bool);
  field->setCategory(i18n(stamp_condition));
  list.append(field);

  field = new Field(QString::fromLatin1("hinged"), i18n("Hinged"));
  field->setCategory(i18n(stamp_condition));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("centering"), i18n("Centering"));
  field->setCategory(i18n(stamp_condition));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("gummed"), i18n("Gummed"));
  field->setCategory(i18n(stamp_condition));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(stamp_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(stamp_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("location"), i18n("Location"));
  field->setCategory(i18n(stamp_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(stamp_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("image"), i18n("Image"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("comments"), i18n("Comments"), Field::Para);
  list.append(field);

  return list;
}

#include "stampcollection.moc"
