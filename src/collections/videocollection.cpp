/***************************************************************************
                             videocollection.cpp
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

#include "videocollection.h"

#include <klocale.h>

#include <qregexp.h>

static const char* video_general = I18N_NOOP("General");
static const char* video_people = I18N_NOOP("Other People");
static const char* video_features = I18N_NOOP("Features");
static const char* video_personal = I18N_NOOP("Personal");

VideoCollection::VideoCollection(bool addAttributes_, const QString& title_ /*=null*/)
   : BCCollection(title_, QString::fromLatin1("video"), i18n("Videos")) {
  setTitle(title_.isNull() ? i18n("My Videos") : title_);
  if(addAttributes_) {
    addAttributes(VideoCollection::defaultAttributes());
  }
  setDefaultGroupAttribute(QString::fromLatin1("genre"));
  setDefaultViewAttributes(QStringList::split(',', QString::fromLatin1("title,genre,medium,director,studio")));
}

BCAttributeList VideoCollection::defaultAttributes() {
  BCAttributeList list;
  BCAttribute* att;

  att = new BCAttribute(QString::fromLatin1("title"), i18n("Title"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::NoDelete);
  att->setFormatFlag(BCAttribute::FormatTitle);
  list.append(att);

  QStringList media;
  media << i18n("DVD") << i18n("VHS") << i18n("VCD") << i18n("DivX");
  att = new BCAttribute(QString::fromLatin1("medium"), i18n("Medium"), media);
  att->setCategory(i18n(video_general));
  att->setFlags(BCAttribute::AllowGrouped);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("year"), i18n("Production Year"), BCAttribute::Number);
  att->setCategory(i18n(video_general));
  att->setFlags(BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  list.append(att);

  QStringList cert = QStringList::split(QRegExp(QString::fromLatin1("\\s*,\\s*")),
                                        i18n("G (USA),PG (USA),PG-13 (USA),R (USA)"), false);
  att = new BCAttribute(QString::fromLatin1("certification"), i18n("Certification"), cert);
  att->setCategory(i18n(video_general));
  att->setFlags(BCAttribute::AllowGrouped);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("genre"), i18n("Genre"));
  att->setCategory(i18n(video_general));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

  QStringList region;
  region << i18n("Region 1")
         << i18n("Region 2")
         << i18n("Region 3")
         << i18n("Region 4")
         << i18n("Region 5")
         << i18n("Region 6")
         << i18n("Region 7")
         << i18n("Region 8");
  att = new BCAttribute(QString::fromLatin1("region"), i18n("Region"), region);
  att->setCategory(i18n(video_general));
  att->setFlags(BCAttribute::AllowGrouped);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("nationality"), i18n("Nationality"));
  att->setCategory(i18n(video_general));
  att->setFlags(BCAttribute::AllowCompletion);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

  QStringList format;
  format << i18n("NTSC") << i18n("PAL") << i18n("SECAM");
  att = new BCAttribute(QString::fromLatin1("format"), i18n("Format"), format);
  att->setCategory(i18n(video_general));
  att->setFlags(BCAttribute::AllowGrouped);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("cast"), i18n("Cast"), BCAttribute::Table2);
  att->setFormatFlag(BCAttribute::FormatName);
  att->setFlags(BCAttribute::AllowGrouped);
  att->setDescription(i18n("A table for the cast members, along with the roles they play"));
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("director"), i18n("Director"));
  att->setCategory(i18n(video_people));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatName);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("producer"), i18n("Producer"));
  att->setCategory(i18n(video_people));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatName);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("writer"), i18n("Writer"));
  att->setCategory(i18n(video_people));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatName);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("composer"), i18n("Composer"));
  att->setCategory(i18n(video_people));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatName);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("studio"), i18n("Studio"));
  att->setCategory(i18n(video_people));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("language"), i18n("Language Tracks"));
  att->setCategory(i18n(video_features));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("subtitle"), i18n("Subtitle Languages"));
  att->setCategory(i18n(video_features));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("audio-track"), i18n("Audio Tracks"));
  att->setCategory(i18n(video_features));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("running-time"), i18n("Running Time"), BCAttribute::Number);
  att->setCategory(i18n(video_features));

  att->setDescription(i18n("The running time of the video (in minutes)"));
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("aspect-ratio"), i18n("Aspect Ratio"));
  att->setCategory(i18n(video_features));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("widescreen"), i18n("Widescreen"), BCAttribute::Bool);
  att->setCategory(i18n(video_features));
  list.append(att);

  QStringList color;
  color << i18n("Color") << i18n("Black & White");
  att = new BCAttribute(QString::fromLatin1("color"), i18n("Color Mode"), color);
  att->setCategory(i18n(video_features));
  att->setFlags(BCAttribute::AllowGrouped);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("directors-cut"), i18n("Director's Cut"), BCAttribute::Bool);
  att->setCategory(i18n(video_features));
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("plot"), i18n("Plot Summary"), BCAttribute::Para);
  list.append(att);

  QStringList prating;
  prating << i18n("5 - Best") << i18n("4 - Good") << i18n("3 - Neutral") << i18n("2 - Bad") << i18n("1 - Worst");
  att = new BCAttribute(QString::fromLatin1("rating"), i18n("Personal Rating"), prating);
  att->setCategory(i18n(video_personal));
  att->setFlags(BCAttribute::AllowGrouped);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  att->setCategory(i18n(video_personal));
  att->setFormatFlag(BCAttribute::FormatDate);
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("gift"), i18n("Gift"), BCAttribute::Bool);
  att->setCategory(i18n(video_personal));
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  att->setCategory(i18n(video_personal));
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("loaned"), i18n("Loaned"), BCAttribute::Bool);
  att->setCategory(i18n(video_personal));
  list.append(att);

  att = new BCAttribute(QString::fromLatin1("comments"), i18n("Comments"));
  att->setCategory(i18n(video_personal));
  list.append(att);

  return list;
}
