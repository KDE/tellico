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

#include "videocollection.h"
#include "../collectionfactory.h"

#include <klocale.h>

#include <qregexp.h>

using Bookcase::Data::VideoCollection;

static const char* video_general = I18N_NOOP("General");
static const char* video_people = I18N_NOOP("Other People");
static const char* video_features = I18N_NOOP("Features");
static const char* video_personal = I18N_NOOP("Personal");

VideoCollection::VideoCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_, CollectionFactory::entryName(Video), i18n("Videos")) {
  setTitle(title_.isNull() ? i18n("My Videos") : title_);
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("genre"));
}

Bookcase::Data::FieldList VideoCollection::defaultFields() {
  FieldList list;
  Field* field;

  field = new Field(QString::fromLatin1("title"), i18n("Title"));
  field->setCategory(i18n("General"));
  field->setFlags(Field::NoDelete);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  QStringList media;
  media << i18n("DVD") << i18n("VHS") << i18n("VCD") << i18n("DivX");
  field = new Field(QString::fromLatin1("medium"), i18n("Medium"), media);
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("year"), i18n("Production Year"), Field::Number);
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  QStringList cert = QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                                        i18n("_:Movie ratings\n"
                                             "G (USA),PG (USA),PG-13 (USA),R (USA)",
                                             "G (USA),PG (USA),PG-13 (USA),R (USA)"),
                                        false);
  field = new Field(QString::fromLatin1("certification"), i18n("Certification"), cert);
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("genre"), i18n("Genre"));
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  QStringList region;
  region << i18n("Region 1")
         << i18n("Region 2")
         << i18n("Region 3")
         << i18n("Region 4")
         << i18n("Region 5")
         << i18n("Region 6")
         << i18n("Region 7")
         << i18n("Region 8");
  field = new Field(QString::fromLatin1("region"), i18n("Region"), region);
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("nationality"), i18n("Nationality"));
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowCompletion);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  QStringList format;
  format << i18n("NTSC") << i18n("PAL") << i18n("SECAM");
  field = new Field(QString::fromLatin1("format"), i18n("Format"), format);
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("cast"), i18n("Cast"), Field::Table2);
  field->setFormatFlag(Field::FormatName);
  field->setFlags(Field::AllowGrouped);
  field->setDescription(i18n("A table for the cast members, along with the roles they play"));
  list.append(field);

  field = new Field(QString::fromLatin1("director"), i18n("Director"));
  field->setCategory(i18n(video_people));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QString::fromLatin1("producer"), i18n("Producer"));
  field->setCategory(i18n(video_people));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QString::fromLatin1("writer"), i18n("Writer"));
  field->setCategory(i18n(video_people));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QString::fromLatin1("composer"), i18n("Composer"));
  field->setCategory(i18n(video_people));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QString::fromLatin1("studio"), i18n("Studio"));
  field->setCategory(i18n(video_people));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("language"), i18n("Language Tracks"));
  field->setCategory(i18n(video_features));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("subtitle"), i18n("Subtitle Languages"));
  field->setCategory(i18n(video_features));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("audio-track"), i18n("Audio Tracks"));
  field->setCategory(i18n(video_features));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("running-time"), i18n("Running Time"), Field::Number);
  field->setCategory(i18n(video_features));

  field->setDescription(i18n("The running time of the video (in minutes)"));
  list.append(field);

  field = new Field(QString::fromLatin1("aspect-ratio"), i18n("Aspect Ratio"));
  field->setCategory(i18n(video_features));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("widescreen"), i18n("Widescreen"), Field::Bool);
  field->setCategory(i18n(video_features));
  list.append(field);

  QStringList color;
  color << i18n("Color") << i18n("Black & White");
  field = new Field(QString::fromLatin1("color"), i18n("Color Mode"), color);
  field->setCategory(i18n(video_features));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("directors-cut"), i18n("Director's Cut"), Field::Bool);
  field->setCategory(i18n(video_features));
  list.append(field);

  field = new Field(QString::fromLatin1("plot"), i18n("Plot Summary"), Field::Para);
  list.append(field);

  QStringList prating;
  prating << i18n("5 - Best") << i18n("4 - Good") << i18n("3 - Neutral") << i18n("2 - Bad") << i18n("1 - Worst");
  field = new Field(QString::fromLatin1("rating"), i18n("Personal Rating"), prating);
  field->setCategory(i18n(video_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(video_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QString::fromLatin1("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(video_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(video_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(video_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(video_personal));
  list.append(field);

  return list;
}
