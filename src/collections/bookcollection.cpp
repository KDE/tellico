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

#include "bookcollection.h"
#include "../collectionfactory.h"

#include <klocale.h>

namespace {
  static const char* book_general = I18N_NOOP("General");
  static const char* book_publishing = I18N_NOOP("Publishing");
  static const char* book_classification = I18N_NOOP("Classification");
  static const char* book_personal = I18N_NOOP("Personal");
}

using Tellico::Data::BookCollection;

BookCollection::BookCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_, CollectionFactory::entryName(Book), i18n("Books")) {
  setTitle(title_.isNull() ? i18n("My Books") : title_);
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("author"));
}

Tellico::Data::FieldList BookCollection::defaultFields() {
  FieldList list;
  Field* field;

  field = new Field(QString::fromLatin1("title"), i18n("Title"));
  field->setCategory(i18n("General"));
  field->setFlags(Field::NoDelete);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("subtitle"), i18n("Subtitle"));
  field->setCategory(i18n(book_general));
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("author"), i18n("Author"));
  field->setCategory(i18n(book_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  QStringList binding;
  binding << i18n("Hardback") << i18n("Paperback") << i18n("Trade Paperback")
          << i18n("E-Book") << i18n("Magazine") << i18n("Journal");
  field = new Field(QString::fromLatin1("binding"), i18n("Binding"), binding);
  field->setCategory(i18n(book_general));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(book_general));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(book_general));
  list.append(field);

  field = new Field(QString::fromLatin1("publisher"), i18n("Publisher"));
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("edition"), i18n("Edition"));
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowCompletion);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("cr_year"), i18n("Copyright Year"), Field::Number);
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowGrouped | Field::AllowMultiple);
  list.append(field);

  field = new Field(QString::fromLatin1("pub_year"), i18n("Publication Year"), Field::Number);
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("isbn"), i18n("ISBN#"));
  field->setCategory(i18n(book_publishing));
  field->setDescription(i18n("International Standard Book Number"));
  list.append(field);

  field = new Field(QString::fromLatin1("lccn"), i18n("LCCN#"));
  field->setCategory(i18n(book_publishing));
  field->setDescription(i18n("Library of Congress Control Number"));
  list.append(field);

  field = new Field(QString::fromLatin1("pages"), i18n("Pages"), Field::Number);
  field->setCategory(i18n(book_publishing));
  list.append(field);

  field = new Field(QString::fromLatin1("language"), i18n("Language"));
  field->setCategory(i18n(book_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  list.append(field);

  field = new Field(QString::fromLatin1("genre"), i18n("Genre"));
  field->setCategory(i18n(book_classification));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  // in document versions < 3, this was "keywords" and not "keyword"
  // but the title didn't change, only the name
  field = new Field(QString::fromLatin1("keyword"), i18n("Keywords"));
  field->setCategory(i18n(book_classification));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("series"), i18n("Series"));
  field->setCategory(i18n(book_classification));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("series_num"), i18n("Series Number"), Field::Number);
  field->setCategory(i18n(book_classification));
  list.append(field);

  QStringList cond;
  cond << i18n("New") << i18n("Used");
  field = new Field(QString::fromLatin1("condition"), i18n("Condition"), cond);
  field->setCategory(i18n(book_classification));
  list.append(field);

  field = new Field(QString::fromLatin1("signed"), i18n("Signed"), Field::Bool);
  field->setCategory(i18n(book_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("read"), i18n("Read"), Field::Bool);
  field->setCategory(i18n(book_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(book_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(book_personal));
  list.append(field);

  QStringList rating;
  rating << i18n("5 - Best") << i18n("4 - Good") << i18n("3 - Neutral") << i18n("2 - Bad") << i18n("1 - Worst");
  field = new Field(QString::fromLatin1("rating"), i18n("Rating"), rating);
  field->setCategory(i18n(book_personal));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("cover"), i18n("Front Cover"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(book_personal));
  list.append(field);

  return list;
}

#include "bookcollection.moc"
