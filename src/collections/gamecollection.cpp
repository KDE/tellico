/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "gamecollection.h"

#include <klocale.h>

namespace {
  static const char* game_general = I18N_NOOP("General");
  static const char* game_personal = I18N_NOOP("Personal");
}

using Tellico::Data::GameCollection;

GameCollection::GameCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_.isEmpty() ? i18n("My Games") : title_) {
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("platform"));
}

Tellico::Data::FieldVec GameCollection::defaultFields() {
  FieldVec list;
  FieldPtr field;

  field = new Field(QString::fromLatin1("title"), i18n("Title"));
  field->setCategory(i18n(game_general));
  field->setFlags(Field::NoDelete);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  QStringList platform;
  platform << i18n("Xbox 360") << i18n("Xbox")
           << i18n("PlayStation3") << i18n("PlayStation2") << i18n("PlayStation") << i18n("PlayStation Portable", "PSP")
           << i18n("Nintendo Wii") << i18n("Nintendo DS") << i18n("GameCube") << i18n("Dreamcast")
           << i18n("Game Boy Advance") << i18n("Game Boy Color") << i18n("Game Boy")
           << i18n("Windows Platform", "Windows") << i18n("Mac OS") << i18n("Linux");
  field = new Field(QString::fromLatin1("platform"), i18n("Platform"), platform);
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("genre"), i18n("Genre"));
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("year"), i18n("Release Year"), Field::Number);
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("publisher"), i18n("Games - Publisher", "Publisher"));
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("developer"), i18n("Developer"));
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  QStringList cert = QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                                        i18n("Video game ratings - "
                                             "Unrated, Adults Only, Mature, Teen, Everyone, Early Childhood, Pending",
                                             "Unrated, Adults Only, Mature, Teen, Everyone, Early Childhood, Pending"),
                                        false);
  field = new Field(QString::fromLatin1("certification"), i18n("ESRB Rating"), cert);
  field->setCategory(i18n(game_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("description"), i18n("Description"), Field::Para);
  list.append(field);

  field = new Field(QString::fromLatin1("rating"), i18n("Personal Rating"), Field::Rating);
  field->setCategory(i18n(game_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("completed"), i18n("Completed"), Field::Bool);
  field->setCategory(i18n(game_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(game_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QString::fromLatin1("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(game_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(game_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(game_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(game_personal));
  list.append(field);

  return list;
}

#include "gamecollection.moc"
