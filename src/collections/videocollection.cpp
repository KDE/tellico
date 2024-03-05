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

#include "videocollection.h"
#include "../entrycomparison.h"
#include "../core/tellico_strings.h"

#include <KLocalizedString>

using Tellico::Data::VideoCollection;

VideoCollection::VideoCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Videos") : title_) {
  setDefaultGroupField(QStringLiteral("genre"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList VideoCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  list.append(Field::createDefaultField(Field::TitleField));

  QStringList media;
  media << i18n("DVD") << i18n("VHS") << i18n("VCD") << i18n("DivX") << i18n("Blu-ray") << i18n("HD DVD");
  field = new Field(QStringLiteral("medium"), i18n("Medium"), media);
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("year"), i18n("Production Year"), Field::Number);
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  auto cert = FieldFormat::splitValue(i18nc("Movie ratings - "
                                            "G (USA),PG (USA),PG-13 (USA),R (USA), U (USA)",
                                            "G (USA),PG (USA),PG-13 (USA),R (USA), U (USA)"),
                                      FieldFormat::CommaRegExpSplit);
  field = new Field(QStringLiteral("certification"), i18n("Certification"), cert);
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("genre"), i18n("Genre"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  QStringList region;
  region << i18n("Region 0")
         << i18n("Region 1")
         << i18n("Region 2")
         << i18n("Region 3")
         << i18n("Region 4")
         << i18n("Region 5")
         << i18n("Region 6")
         << i18n("Region 7")
         << i18n("Region 8");
  field = new Field(QStringLiteral("region"), i18n("Region"), region);
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("nationality"), i18n("Nationality"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  QStringList format;
  format << i18n("NTSC") << i18n("PAL") << i18n("SECAM");
  field = new Field(QStringLiteral("format"), i18n("Format"), format);
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("cast"), i18n("Cast"), Field::Table);
  field->setProperty(QStringLiteral("columns"), QStringLiteral("2"));
  field->setProperty(QStringLiteral("column1"), i18n("Actor/Actress"));
  field->setProperty(QStringLiteral("column2"), i18n("Role"));
  field->setFormatType(FieldFormat::FormatName);
  field->setFlags(Field::AllowGrouped);
  field->setDescription(i18n("A table for the cast members, along with the roles they play"));
  list.append(field);

  field = new Field(QStringLiteral("director"), i18n("Director"));
  field->setCategory(TC_I18N1(categoryPeople));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("producer"), i18n("Producer"));
  field->setCategory(TC_I18N1(categoryPeople));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("writer"), i18n("Writer"));
  field->setCategory(TC_I18N1(categoryPeople));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("composer"), i18n("Composer"));
  field->setCategory(TC_I18N1(categoryPeople));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("studio"), i18n("Studio"));
  field->setCategory(TC_I18N1(categoryPeople));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("language"), i18n("Language Tracks"));
  field->setCategory(TC_I18N1(categoryFeatures));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("subtitle"), i18n("Subtitle Languages"));
  field->setCategory(TC_I18N1(categoryFeatures));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("audio-track"), i18n("Audio Tracks"));
  field->setCategory(TC_I18N1(categoryFeatures));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("running-time"), i18n("Running Time"), Field::Number);
  field->setCategory(TC_I18N1(categoryFeatures));
  field->setDescription(i18n("The running time of the video (in minutes)"));
  list.append(field);

  field = new Field(QStringLiteral("aspect-ratio"), i18n("Aspect Ratio"));
  field->setCategory(TC_I18N1(categoryFeatures));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("widescreen"), i18n("Widescreen"), Field::Bool);
  field->setCategory(TC_I18N1(categoryFeatures));
  list.append(field);

  QStringList color;
  color << i18n("Color") << i18n("Black & White");
  field = new Field(QStringLiteral("color"), i18n("Color Mode"), color);
  field->setCategory(TC_I18N1(categoryFeatures));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("directors-cut"), i18n("Director's Cut"), Field::Bool);
  field->setCategory(TC_I18N1(categoryFeatures));
  list.append(field);

  field = new Field(QStringLiteral("plot"), i18n("Plot Summary"), Field::Para);
  list.append(field);

  field = new Field(QStringLiteral("rating"), i18n("Personal Rating"), Field::Rating);
  field->setCategory(TC_I18N1(categoryPersonal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("pur_date"), i18n("Purchase Date"));
  field->setCategory(TC_I18N1(categoryPersonal));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QStringLiteral("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("pur_price"), i18n("Purchase Price"));
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(TC_I18N1(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("keyword"), i18n("Keywords"));
  field->setCategory(TC_I18N1(categoryPersonal));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("cover"), i18n("Cover"), Field::Image);
  list.append(field);

  field = new Field(QStringLiteral("comments"), i18n("Comments"), Field::Para);
  list.append(field);

  list.append(Field::createDefaultField(Field::IDField));
  list.append(Field::createDefaultField(Field::CreatedDateField));
  list.append(Field::createDefaultField(Field::ModifiedDateField));

  return list;
}

int VideoCollection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  // when imdb field is equal it is the same
  if(EntryComparison::score(entry1_, entry2_, QStringLiteral("imdb"), this) > 0) {
    return EntryComparison::ENTRY_PERFECT_MATCH;
  }
  // not enough for title to be equal, must also have another field
  // ever possible for a studio to do two movies with identical titles?
  int res = 0;
  res += EntryComparison::MATCH_WEIGHT_HIGH*EntryComparison::score(entry1_, entry2_, QStringLiteral("title"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("year"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("director"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("studio"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("medium"), this);
  return res;
}
