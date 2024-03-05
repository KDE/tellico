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

#include "bookcollection.h"
#include "../entrycomparison.h"
#include "../core/tellico_strings.h"

#include <KLocalizedString>

using Tellico::Data::BookCollection;

BookCollection::BookCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Books") : title_) {
  setDefaultGroupField(QStringLiteral("author"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList BookCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  list.append(Field::createDefaultField(Field::TitleField));

  field = new Field(QStringLiteral("subtitle"), i18n("Subtitle"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QStringLiteral("author"), i18n("Author"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("editor"), i18n("Editor"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  QStringList binding;
  binding << i18n("Hardback") << i18n("Paperback") << i18n("Trade Paperback")
          << i18n("E-Book") << i18n("Magazine") << i18n("Journal");
  field = new Field(QStringLiteral("binding"), i18n("Binding"), binding);
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("pur_date"), i18n("Purchase Date"));
  field->setCategory(TC_I18N(categoryGeneral));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QStringLiteral("pur_price"), i18n("Purchase Price"));
  field->setCategory(TC_I18N(categoryGeneral));
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

  field = new Field(QStringLiteral("cr_year"), i18n("Copyright Year"), Field::Number);
  field->setCategory(TC_I18N(categoryPublishing));
  field->setFlags(Field::AllowGrouped | Field::AllowMultiple);
  list.append(field);

  field = new Field(QStringLiteral("pub_year"), i18n("Publication Year"), Field::Number);
  field->setCategory(TC_I18N(categoryPublishing));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  list.append(Field::createDefaultField(Field::IsbnField));

  field = new Field(QStringLiteral("lccn"), i18n("LCCN#"));
  field->setCategory(TC_I18N(categoryPublishing));
  field->setDescription(i18n("Library of Congress Control Number"));
  list.append(field);

  field = new Field(QStringLiteral("pages"), i18n("Pages"), Field::Number);
  field->setCategory(TC_I18N(categoryPublishing));
  list.append(field);

  field = new Field(QStringLiteral("translator"), i18n("Translator"));
  field->setCategory(TC_I18N(categoryPublishing));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("language"), i18n("Language"));
  field->setCategory(TC_I18N(categoryPublishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  list.append(field);

  field = new Field(QStringLiteral("genre"), i18n("Genre"));
  field->setCategory(TC_I18N(categoryClassification));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  // in document versions < 3, this was "keywords" and not "keyword"
  // but the title didn't change, only the name
  field = new Field(QStringLiteral("keyword"), i18n("Keywords"));
  field->setCategory(TC_I18N(categoryClassification));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("series"), i18n("Series"));
  field->setCategory(TC_I18N(categoryClassification));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("series_num"), i18n("Series Number"), Field::Number);
  field->setCategory(TC_I18N(categoryClassification));
  list.append(field);

  QStringList cond;
  cond << i18n("New") << i18n("Used");
  field = new Field(QStringLiteral("condition"), i18n("Condition"), cond);
  field->setCategory(TC_I18N(categoryClassification));
  list.append(field);

  field = new Field(QStringLiteral("signed"), i18n("Signed"), Field::Bool);
  field->setCategory(TC_I18N(categoryPersonal));
  list.append(field);

  field = new Field(QStringLiteral("read"), i18n("Read"), Field::Bool);
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

int BookCollection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  if(!entry1_ || !entry2_) {
    return 0;
  }
  // equal isbn's or lccn's are easy
  if(EntryComparison::score(entry1_, entry2_, QStringLiteral("isbn"), this) > 0 ||
     EntryComparison::score(entry1_, entry2_, QStringLiteral("lccn"), this) > 0) {
    return EntryComparison::ENTRY_PERFECT_MATCH;
  }
  int res = 0;
  res += EntryComparison::MATCH_WEIGHT_HIGH*EntryComparison::score(entry1_, entry2_, QStringLiteral("title"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("author"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("cr_year"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("pub_year"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("publisher"), this);
  res += EntryComparison::MATCH_WEIGHT_LOW *EntryComparison::score(entry1_, entry2_, QStringLiteral("binding"), this);
  return res;
}
