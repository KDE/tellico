/***************************************************************************
                             musiccollection.cpp
                             -------------------
    begin                : Tue Mar 4 2003
    copyright            : (C) 2003 by Robby Stephenson
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

#include <klocale.h>

static const char* music_general = I18N_NOOP("General");
static const char* music_personal = I18N_NOOP("Personal");

MusicCollection::MusicCollection(bool addAttributes_, const QString& title_ /*=null*/)
   : BCCollection(title_, QString::fromLatin1("album"), i18n("Albums")) {
  setTitle(title_.isNull() ? i18n("My Music") : title_);
  if(addAttributes_) {
    addAttributes(MusicCollection::defaultAttributes());
  }
  setDefaultGroupAttribute(QString::fromLatin1("artist"));
  setDefaultViewAttributes(QStringList::split(',', QString::fromLatin1("title,artist,album,genre")));
}

BCAttributeList MusicCollection::defaultAttributes() {
  BCAttributeList list;
  BCAttribute* att;

  att = new BCAttribute(QString::fromLatin1("title"), i18n("Title"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::NoDelete);
  att->setFormatFlag(BCAttribute::FormatTitle);
  list.append(att);

  QStringList media;
  media << i18n("Compact Disc") << i18n("Cassette") << i18n("Vinyl");
  att = new BCAttribute(QString::fromLatin1("medium"), i18n("Medium"), media);
  att->setCategory(i18n(music_general));
  att->setFlags(BCAttribute::AllowGrouped);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("artist"), i18n("Artist"));
  att->setCategory(i18n(music_general));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped | BCAttribute::AllowMultiple);
  att->setFormatFlag(BCAttribute::FormatName);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("album"), i18n("Album"));
  att->setCategory(i18n(music_general));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatTitle);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("label"), i18n("Label"));
  att->setCategory(i18n(music_general));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped | BCAttribute::AllowMultiple);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("year"), i18n("Year"), BCAttribute::Number);
  att->setCategory(i18n(music_general));
  att->setFlags(BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("genre"), i18n("Genre"));
  att->setCategory(i18n(music_general));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("track"), i18n("Tracks"), BCAttribute::Table);
  att->setFormatFlag(BCAttribute::FormatTitle);
  list.append(att);

  QStringList rating;
  rating << i18n("5 - Best") << i18n("4 - Good") << i18n("3 - Neutral") << i18n("2 - Bad") << i18n("1 - Worst");
  att = new BCAttribute(QString::fromLatin1("rating"), i18n("Rating"), rating);
  att->setCategory(i18n(music_personal));
  att->setFlags(BCAttribute::AllowGrouped);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  att->setCategory(i18n(music_personal));
  att->setFormatFlag(BCAttribute::FormatDate);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("gift"), i18n("Gift"), BCAttribute::Bool);
  att->setCategory(i18n(music_personal));
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  att->setCategory(i18n(music_personal));
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("loaned"), i18n("Loaned"), BCAttribute::Bool);
  att->setCategory(i18n(music_personal));
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("comments"), i18n("Comments"));
  att->setCategory(i18n(music_personal));
  list.append(att);

  return list;
}
