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

#include "comicbookcollection.h"
#include "../collectionfactory.h"

#include <klocale.h>

using Bookcase::Data::ComicBookCollection;

static const QString comic_general = QString::fromLatin1(I18N_NOOP("General"));
static const QString comic_publishing = QString::fromLatin1(I18N_NOOP("Publishing"));
static const QString comic_classification = QString::fromLatin1(I18N_NOOP("Classification"));
static const QString comic_personal = QString::fromLatin1(I18N_NOOP("Personal"));

ComicBookCollection::ComicBookCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_, CollectionFactory::entryName(ComicBook), i18n("Comics")) {
  setTitle(title_.isNull() ? i18n("My Comic Books") : title_);
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("series"));
}

Bookcase::Data::FieldList ComicBookCollection::defaultFields() {
  FieldList list;
  Field* field;

  field = new Field(QString::fromLatin1("title"), i18n("Title"));
  field->setCategory(i18n(comic_general));
  field->setFlags(Field::NoDelete);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("subtitle"), i18n("Subtitle"));
  field->setCategory(i18n(comic_general));
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("writer"), i18n("Writer"));
  field->setCategory(i18n(comic_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QString::fromLatin1("artist"), i18n("Artist"));
  field->setCategory(i18n(comic_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QString::fromLatin1("series"), i18n("Series"));
  field->setCategory(i18n(comic_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("issue"), i18n("Issue"), Field::Number);
  field->setCategory(i18n(comic_general));
  field->setFlags(Field::AllowMultiple);
  list.append(field);

  field = new Field(QString::fromLatin1("publisher"), i18n("Publisher"));
  field->setCategory(i18n(comic_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("edition"), i18n("Edition"));
  field->setCategory(i18n(comic_publishing));
  field->setFlags(Field::AllowCompletion);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("pub_year"), i18n("Publication Year"),  Field::Number);
  field->setCategory(i18n(comic_publishing));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("pages"), i18n("Pages"), Field::Number);
  field->setCategory(i18n(comic_publishing));
  list.append(field);

  field = new Field(QString::fromLatin1("country"), i18n("Country"));
  field->setCategory(i18n(comic_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("language"), i18n("Language"));
  field->setCategory(i18n(comic_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped | Field::AllowMultiple);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("genre"), i18n("Genre"));
  field->setCategory(i18n(comic_classification));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("keyword"), i18n("Keywords"));
  field->setCategory(i18n(comic_classification));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  QStringList cond;
  cond << i18n("Mint") << i18n("Near Mint") << i18n("Very Fine") << i18n("Fine")
       << i18n("Very Good") << i18n("Good") << i18n("Fair") << i18n("Poor");
  field = new Field(QString::fromLatin1("condition"), i18n("Condition"), cond);
  field->setCategory(i18n(comic_classification));
  list.append(field);

  field = new Field(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(comic_personal));
  field->setFormatFlag(Field::FormatDate);
  list.append(field);

  field = new Field(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(comic_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("signed"), i18n("Signed"), Field::Bool);
  field->setCategory(i18n(comic_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(comic_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("loaned"), i18n("Loaned"), Field::Bool);
  field->setCategory(i18n(comic_personal));
  list.append(field);

  field = new Field(QString::fromLatin1("cover"), i18n("Front Cover"), Field::Image);
  list.append(field);

  field = new Field(QString::fromLatin1("comments"), i18n("Comments"), Field::Para);
  field->setCategory(i18n(comic_personal));
  list.append(field);

  return list;
}
