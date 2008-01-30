/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson, Steve Beattie
    email                : robby@periapsis.org, sbeattie@suse.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "boardgamecollection.h"

#include <klocale.h>

namespace {
  static const char* boardgame_general = I18N_NOOP("General");
  static const char* boardgame_personal = I18N_NOOP("Personal");
}

using Tellico::Data::BoardGameCollection;

BoardGameCollection::BoardGameCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_.isEmpty() ? i18n("My Board Games") : title_) {
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("genre"));
}

Tellico::Data::FieldVec BoardGameCollection::defaultFields() {
  FieldVec list;
  FieldPtr field;

  field = new Field(QString::fromLatin1("title"), i18n("Title"));
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::NoDelete);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("genre"), i18n("Genre"));
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("mechanism"), i18n("Mechanism"));
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("year"), i18n("Release Year"), Field::Number);
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("publisher"), i18n("Publisher"));
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("designer"), i18n("Designer"));
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("num-player"), i18n("Number of Players"), Field::Number);
  field->setCategory(i18n(boardgame_general));
  field->setFlags(Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("description"), i18n("Description"), Field::Para);
  list.append(field);

  field = new Field(QString::fromLatin1("rating"), i18n("Rating"), Field::Rating);
  field->setCategory(i18n(boardgame_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(boardgame_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QString::fromLatin1("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(boardgame_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(boardgame_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(boardgame_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(boardgame_personal));
  list.append(field);

  return list;
}

#include "boardgamecollection.moc"
