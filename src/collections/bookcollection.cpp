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

#include <klocale.h>

namespace {
  static const char* book_general = I18N_NOOP("General");
  static const char* book_publishing = I18N_NOOP("Publishing");
  static const char* book_classification = I18N_NOOP("Classification");
  static const char* book_personal = I18N_NOOP("Personal");
}

using Tellico::Data::BookCollection;

BookCollection::BookCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Books") : title_) {
  setDefaultGroupField(QLatin1String("author"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList BookCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  list.append(createDefaultField(TitleField));

  field = new Field(QLatin1String("subtitle"), i18n("Subtitle"));
  field->setCategory(i18n(book_general));
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QLatin1String("author"), i18n("Author"));
  field->setCategory(i18n(book_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QLatin1String("editor"), i18n("Editor"));
  field->setCategory(i18n(book_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  QStringList binding;
  binding << i18n("Hardback") << i18n("Paperback") << i18n("Trade Paperback")
          << i18n("E-Book") << i18n("Magazine") << i18n("Journal");
  field = new Field(QLatin1String("binding"), i18n("Binding"), binding);
  field->setCategory(i18n(book_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(book_general));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QLatin1String("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(book_general));
  list.append(field);

  field = new Field(QLatin1String("publisher"), i18n("Publisher"));
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("edition"), i18n("Edition"));
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowCompletion);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("cr_year"), i18n("Copyright Year"), Field::Number);
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowGrouped | Field::AllowMultiple);
  list.append(field);

  field = new Field(QLatin1String("pub_year"), i18n("Publication Year"), Field::Number);
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("isbn"), i18n("ISBN#"));
  field->setCategory(i18n(book_publishing));
  field->setDescription(i18n("International Standard Book Number"));
  list.append(field);

  field = new Field(QLatin1String("lccn"), i18n("LCCN#"));
  field->setCategory(i18n(book_publishing));
  field->setDescription(i18n("Library of Congress Control Number"));
  list.append(field);

  field = new Field(QLatin1String("pages"), i18n("Pages"), Field::Number);
  field->setCategory(i18n(book_publishing));
  list.append(field);

  field = new Field(QLatin1String("translator"), i18n("Translator"));
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QLatin1String("language"), i18n("Language"));
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  list.append(field);

  field = new Field(QLatin1String("genre"), i18n("Genre"));
  field->setCategory(i18n(book_classification));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  // in document versions < 3, this was "keywords" and not "keyword"
  // but the title didn't change, only the name
  field = new Field(QLatin1String("keyword"), i18n("Keywords"));
  field->setCategory(i18n(book_classification));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("series"), i18n("Series"));
  field->setCategory(i18n(book_classification));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("series_num"), i18n("Series Number"), Field::Number);
  field->setCategory(i18n(book_classification));
  list.append(field);

  QStringList cond;
  cond << i18n("New") << i18n("Used");
  field = new Field(QLatin1String("condition"), i18n("Condition"), cond);
  field->setCategory(i18n(book_classification));
  list.append(field);

  field = new Field(QLatin1String("signed"), i18n("Signed"), Field::Bool);
  field->setCategory(i18n(book_personal));
  list.append(field);

  field = new Field(QLatin1String("read"), i18n("Read"), Field::Bool);
  field->setCategory(i18n(book_personal));
  list.append(field);

  field = new Field(QLatin1String("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(book_personal));
  list.append(field);

  field = new Field(QLatin1String("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(book_personal));
  list.append(field);

  field = new Field(QLatin1String("rating"), i18n("Rating"), Field::Rating);
  field->setCategory(i18n(book_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("cover"), i18n("Front Cover"), Field::Image);
  list.append(field);

  field = new Field(QLatin1String("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(book_personal));
  list.append(field);

  list.append(createDefaultField(IDField));
  list.append(createDefaultField(CreatedDateField));
  list.append(createDefaultField(ModifiedDateField));

  return list;
}

int BookCollection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  // equal isbn's or lccn's are easy, give it a weight of 100
  if(EntryComparison::score(entry1_, entry2_, QLatin1String("isbn"), this) > 0 ||
     EntryComparison::score(entry1_, entry2_, QLatin1String("lccn"), this) > 0) {
    return 100; // good match
  }
  int res = 3*EntryComparison::score(entry1_, entry2_, QLatin1String("title"), this);
//  if(res == 0) {
//    myDebug() << "different titles for " << entry1_->title() << " vs. "
//              << entry2_->title();
//  }
  res += 2*EntryComparison::score(entry1_, entry2_, QLatin1String("author"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("cr_year"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("pub_year"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("binding"), this);
  return res;
}

#include "bookcollection.moc"
