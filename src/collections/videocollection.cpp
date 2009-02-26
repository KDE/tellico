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

#include "videocollection.h"
#include "../entrycomparison.h"

#include <klocale.h>

namespace {
  static const char* video_general = I18N_NOOP("General");
  static const char* video_people = I18N_NOOP("Other People");
  static const char* video_features = I18N_NOOP("Features");
  static const char* video_personal = I18N_NOOP("Personal");
}

using Tellico::Data::VideoCollection;

VideoCollection::VideoCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_.isEmpty() ? i18n("My Videos") : title_) {
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QLatin1String("genre"));
}

Tellico::Data::FieldList VideoCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = new Field(QLatin1String("title"), i18n("Title"));
  field->setCategory(i18n("General"));
  field->setFlags(Field::NoDelete);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  QStringList media;
  media << i18n("DVD") << i18n("VHS") << i18n("VCD") << i18n("DivX") << i18n("Blu-ray") << i18n("HD DVD");
  field = new Field(QLatin1String("medium"), i18n("Medium"), media);
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("year"), i18n("Production Year"), Field::Number);
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  QStringList cert = i18nc("Movie ratings - "
                           "G (USA),PG (USA),PG-13 (USA),R (USA), U (USA)",
                           "G (USA),PG (USA),PG-13 (USA),R (USA), U (USA)")
                     .split(QRegExp(QLatin1String("\\s*,\\s*")), QString::SkipEmptyParts);
  field = new Field(QLatin1String("certification"), i18n("Certification"), cert);
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("genre"), i18n("Genre"));
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
  field = new Field(QLatin1String("region"), i18n("Region"), region);
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("nationality"), i18n("Nationality"));
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  QStringList format;
  format << i18n("NTSC") << i18n("PAL") << i18n("SECAM");
  field = new Field(QLatin1String("format"), i18n("Format"), format);
  field->setCategory(i18n(video_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("cast"), i18n("Cast"), Field::Table);
  field->setProperty(QLatin1String("columns"), QLatin1String("2"));
  field->setProperty(QLatin1String("column1"), i18n("Actor/Actress"));
  field->setProperty(QLatin1String("column2"), i18n("Role"));
  field->setFormatFlag(Field::FormatName);
  field->setFlags(Field::AllowGrouped);
  field->setDescription(i18n("A table for the cast members, along with the roles they play"));
  list.append(field);

  field = new Field(QLatin1String("director"), i18n("Director"));
  field->setCategory(i18n(video_people));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QLatin1String("producer"), i18n("Producer"));
  field->setCategory(i18n(video_people));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QLatin1String("writer"), i18n("Writer"));
  field->setCategory(i18n(video_people));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QLatin1String("composer"), i18n("Composer"));
  field->setCategory(i18n(video_people));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QLatin1String("studio"), i18n("Studio"));
  field->setCategory(i18n(video_people));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("language"), i18n("Language Tracks"));
  field->setCategory(i18n(video_features));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("subtitle"), i18n("Subtitle Languages"));
  field->setCategory(i18n(video_features));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("audio-track"), i18n("Audio Tracks"));
  field->setCategory(i18n(video_features));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("running-time"), i18n("Running Time"), Field::Number);
  field->setCategory(i18n(video_features));
  field->setDescription(i18n("The running time of the video (in minutes)"));
  list.append(field);

  field = new Field(QLatin1String("aspect-ratio"), i18n("Aspect Ratio"));
  field->setCategory(i18n(video_features));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("widescreen"), i18n("Widescreen"), Field::Bool);
  field->setCategory(i18n(video_features));
  list.append(field);

  QStringList color;
  color << i18n("Color") << i18n("Black & White");
  field = new Field(QLatin1String("color"), i18n("Color Mode"), color);
  field->setCategory(i18n(video_features));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("directors-cut"), i18n("Director's Cut"), Field::Bool);
  field->setCategory(i18n(video_features));
  list.append(field);

  field = new Field(QLatin1String("plot"), i18n("Plot Summary"), Field::Para);
  list.append(field);

  field = new Field(QLatin1String("rating"), i18n("Personal Rating"), Field::Rating);
  field->setCategory(i18n(video_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(video_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QLatin1String("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(video_personal));
  list.append(field);

  field = new Field(QLatin1String("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(video_personal));
  list.append(field);

  field = new Field(QLatin1String("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(video_personal));
  list.append(field);

  field = new Field(QLatin1String("keyword"), i18n("Keywords"));
  field->setCategory(i18n(video_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QLatin1String("comments"), i18n("Comments"), Field::Para);
  list.append(field);

  return list;
}

int VideoCollection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  // not enough for title to be equal, must also have another field
  // ever possible for a studio to do two movies with identical titles?
  int res = 3*EntryComparison::score(entry1_, entry2_, QLatin1String("title"), this);
//  if(res == 0) {
//    myDebug() << "VideoCollection::sameEntry() - different titles for " << entry1_->title() << " vs. "
//              << entry2_->title() << endl;
//  }
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("year"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("director"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("studio"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("medium"), this);
  return res;
}

#include "videocollection.moc"
