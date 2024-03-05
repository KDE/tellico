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
#include "../core/tellico_strings.h"

#include <KLocalizedString>

using Tellico::Data::MusicCollection;

MusicCollection::MusicCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Music") : title_) {
  setDefaultGroupField(QStringLiteral("artist"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList MusicCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = Field::createDefaultField(Field::TitleField);
  field->setTitle(i18n("Album"));
  list.append(field);

  QStringList media;
  media << i18n("Compact Disc") << i18n("DVD") << i18n("Cassette") << i18n("Vinyl");
  field = new Field(QStringLiteral("medium"), i18n("Medium"), media);
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("artist"), i18n("Artist"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  field->setFormatType(FieldFormat::FormatTitle); // don't use FormatName
  list.append(field);

  field = new Field(QStringLiteral("label"), i18n("Label"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("year"), i18n("Year"), Field::Number);
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("genre"), i18n("Genre"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("track"), i18n("Tracks"), Field::Table);
  field->setFormatType(FieldFormat::FormatTitle);
  field->setProperty(QStringLiteral("columns"), QStringLiteral("3"));
  field->setProperty(QStringLiteral("column1"), i18n("Title"));
  field->setProperty(QStringLiteral("column2"), i18n("Artist"));
  field->setProperty(QStringLiteral("column3"), i18n("Length"));
  list.append(field);

  field = new Field(QStringLiteral("rating"), i18n("Rating"), Field::Rating);
  field->setCategory(TC_I18N(categoryPersonal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("pur_date"), i18n("Purchase Date"));
  field->setCategory(TC_I18N(categoryPersonal));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QStringLiteral("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("pur_price"), i18n("Purchase Price"));
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("keyword"), i18n("Keywords"));
  field->setCategory(TC_I18N(categoryPersonal));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QStringLiteral("comments"), i18n("Comments"), Field::Para);
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  list.append(Field::createDefaultField(Field::IDField));
  list.append(Field::createDefaultField(Field::CreatedDateField));
  list.append(Field::createDefaultField(Field::ModifiedDateField));

  return list;
}

int MusicCollection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  // not enough for title to be equal, must also have another field
  int res = 0;
  res += EntryComparison::MATCH_WEIGHT_MED*EntryComparison::score(entry1_, entry2_, QStringLiteral("title"), this);
  res += EntryComparison::MATCH_WEIGHT_MED*EntryComparison::score(entry1_, entry2_, QStringLiteral("artist"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW*EntryComparison::score(entry1_, entry2_, QStringLiteral("year"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW*EntryComparison::score(entry1_, entry2_, QStringLiteral("label"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW*EntryComparison::score(entry1_, entry2_, QStringLiteral("medium"), this);
  return res;
}
