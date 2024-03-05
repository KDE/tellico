/***************************************************************************
    Copyright (C) 2003-2016 Robby Stephenson <robby@periapsis.org>
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

#include "comicbookcollection.h"
#include "../entrycomparison.h"
#include "../core/tellico_strings.h"

#include <KLocalizedString>

using Tellico::Data::ComicBookCollection;

ComicBookCollection::ComicBookCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Comic Books") : title_) {
  setDefaultGroupField(QStringLiteral("series"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList ComicBookCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  list.append(Field::createDefaultField(Field::TitleField));

  field = new Field(QStringLiteral("subtitle"), i18n("Subtitle"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QStringLiteral("writer"), i18n("Writer"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("artist"), i18nc("Comic Book Illustrator", "Artist"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("series"), i18n("Series"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QStringLiteral("issue"), i18n("Issue"), Field::Number);
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowMultiple);
  list.append(field);

  field = new Field(QStringLiteral("publisher"), i18n("Publisher"));
  field->setCategory(TC_I18N(categoryPublishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("edition"), i18n("Edition"));
  field->setCategory(TC_I18N(categoryPublishing));
  field->setFlags(Field::AllowCompletion);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("pub_year"), i18n("Publication Year"),  Field::Number);
  field->setCategory(TC_I18N(categoryPublishing));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("pages"), i18n("Pages"), Field::Number);
  field->setCategory(TC_I18N(categoryPublishing));
  list.append(field);

  field = new Field(QStringLiteral("country"), i18n("Country"));
  field->setCategory(TC_I18N(categoryPublishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("language"), i18n("Language"));
  field->setCategory(TC_I18N(categoryPublishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("genre"), i18n("Genre"));
  field->setCategory(TC_I18N(categoryClassification));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("keyword"), i18n("Keywords"));
  field->setCategory(TC_I18N(categoryClassification));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  auto cond = FieldFormat::splitValue(i18nc("Comic book grade levels - "
                                            "Mint,Near Mint,Very Fine,Fine,Very Good,Good,Fair,Poor",
                                            "Mint,Near Mint,Very Fine,Fine,Very Good,Good,Fair,Poor"),
                                      FieldFormat::CommaRegExpSplit);
  field = new Field(QStringLiteral("condition"), i18n("Condition"), cond);
  field->setCategory(TC_I18N(categoryClassification));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("pur_date"), i18n("Purchase Date"));
  field->setCategory(TC_I18N(categoryPersonal));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QStringLiteral("pur_price"), i18n("Purchase Price"));
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("signed"), i18n("Signed"), Field::Bool);
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("rating"), i18n("Rating"), Field::Rating);
  field->setCategory(TC_I18N(categoryPersonal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  list.append(Field::createDefaultField(Field::FrontCoverField));

  field = new Field(QStringLiteral("plot"), i18n("Plot Summary"), Field::Para);
  list.append(field);

  field = new Field(QStringLiteral("comments"), i18n("Comments"), Field::Para);
  list.append(field);

  list.append(Field::createDefaultField(Field::IDField));
  list.append(Field::createDefaultField(Field::CreatedDateField));
  list.append(Field::createDefaultField(Field::ModifiedDateField));

  return list;
}

int ComicBookCollection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  if(!entry1_ || !entry2_) {
    return 0;
  }

  if(EntryComparison::score(entry1_, entry2_, QStringLiteral("isbn"), this) > 0) {
    return EntryComparison::ENTRY_PERFECT_MATCH;
  }

  int res = 0;
  res += EntryComparison::MATCH_WEIGHT_HIGH*EntryComparison::score(entry1_, entry2_, QStringLiteral("title"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_MED *EntryComparison::score(entry1_, entry2_, QStringLiteral("series"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("pub_year"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_MED *EntryComparison::score(entry1_, entry2_, QStringLiteral("writer"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("artist"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("issue"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("publisher"), this);
  return res;
}
