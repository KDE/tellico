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

#include "musiccollection.h"
#include "../collectionfactory.h"

#include <klocale.h>

using Bookcase::Data::MusicCollection;

static const char* music_general = I18N_NOOP("General");
static const char* music_personal = I18N_NOOP("Personal");

MusicCollection::MusicCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_, CollectionFactory::entryName(Album), i18n("Albums")) {
  setTitle(title_.isNull() ? i18n("My Music") : title_);
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("artist"));
}

Bookcase::Data::FieldList MusicCollection::defaultFields() {
  FieldList list;
  Field* field;

  field = new Field(QString::fromLatin1("title"), i18n("Album"));
  field->setCategory(i18n(music_general));
  field->setFlags(Field::NoDelete | Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  QStringList media;
  media << i18n("Compact Disc") << i18n("Cassette") << i18n("Vinyl");
  field = new Field(QString::fromLatin1("medium"), i18n("Medium"), media);
  field->setCategory(i18n(music_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("artist"), i18n("Artist"));
  field->setCategory(i18n(music_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  field->setFormatFlag(Field::FormatTitle); // don't use FormatName
  list.append(field);

  field = new Field(QString::fromLatin1("label"), i18n("Label"));
  field->setCategory(i18n(music_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("year"), i18n("Year"), Field::Number);
  field->setCategory(i18n(music_general));
  field->setFlags(Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("genre"), i18n("Genre"));
  field->setCategory(i18n(music_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("track"), i18n("Tracks"), Field::Table);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  QStringList rating;
  rating << i18n("5 - Best") << i18n("4 - Good") << i18n("3 - Neutral") << i18n("2 - Bad") << i18n("1 - Worst");
  field = new Field(QString::fromLatin1("rating"), i18n("Rating"), rating);
  field->setCategory(i18n(music_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(music_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QString::fromLatin1("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(music_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(music_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(music_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("keyword"), i18n("Keywords"));
  field->setCategory(i18n(music_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(music_personal));
  list.append(field);

  return list;
}
