/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "musiccollection.h"
#include "../entrycomparison.h"

#include <klocale.h>

namespace {
  static const char* music_general = I18N_NOOP("General");
  static const char* music_personal = I18N_NOOP("Personal");
}

using Tellico::Data::MusicCollection;

MusicCollection::MusicCollection(bool addDefaultFields_, const QString& title_)
   : Collection(addDefaultFields_, title_.isEmpty() ? i18n("My Music") : title_) {
  setDefaultGroupField(QLatin1String("artist"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList MusicCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = new Field(QLatin1String("id"), i18nc("ID # of the entry", "ID"), Field::Number);
  field->setCategory(i18n(music_general));
  field->setDescription(QLatin1String("%{@id}"));
  field->setFlags(Field::Derived);
  field->setFormatFlag(Field::FormatNone);
  list.append(field);

  field = new Field(QLatin1String("title"), i18n("Album"));
  field->setCategory(i18n(music_general));
  field->setFlags(Field::NoDelete | Field::AllowCompletion);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  QStringList media;
  media << i18n("Compact Disc") << i18n("DVD") << i18n("Cassette") << i18n("Vinyl");
  field = new Field(QLatin1String("medium"), i18n("Medium"), media);
  field->setCategory(i18n(music_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("artist"), i18n("Artist"));
  field->setCategory(i18n(music_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  field->setFormatFlag(Field::FormatTitle); // don't use FormatName
  list.append(field);

  field = new Field(QLatin1String("label"), i18n("Label"));
  field->setCategory(i18n(music_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("year"), i18n("Year"), Field::Number);
  field->setCategory(i18n(music_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("genre"), i18n("Genre"));
  field->setCategory(i18n(music_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("track"), i18n("Tracks"), Field::Table);
  field->setFormatFlag(Field::FormatTitle);
  field->setProperty(QLatin1String("columns"), QLatin1String("3"));
  field->setProperty(QLatin1String("column1"), i18n("Title"));
  field->setProperty(QLatin1String("column2"), i18n("Artist"));
  field->setProperty(QLatin1String("column3"), i18n("Length"));
  list.append(field);

  field = new Field(QLatin1String("rating"), i18n("Rating"), Field::Rating);
  field->setCategory(i18n(music_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(music_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QLatin1String("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(music_personal));
  list.append(field);

  field = new Field(QLatin1String("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(music_personal));
  list.append(field);

  field = new Field(QLatin1String("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(music_personal));
  list.append(field);

  field = new Field(QLatin1String("keyword"), i18n("Keywords"));
  field->setCategory(i18n(music_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QLatin1String("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(music_personal));
  list.append(field);

  return list;
}

int MusicCollection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  // not enough for title to be equal, must also have another field
  int res = 2*EntryComparison::score(entry1_, entry2_, QLatin1String("title"), this);
//  if(res == 0) {
//    myDebug() << "different titles for " << entry1_->title() << " vs. "
//              << entry2_->title();
//  }
  res += 2*EntryComparison::score(entry1_, entry2_, QLatin1String("artist"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("year"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("label"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("medium"), this);
  return res;
}

#include "musiccollection.moc"
