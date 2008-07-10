/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@priapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "filecatalog.h"

#include <klocale.h>

namespace {
  static const char* file_general = I18N_NOOP("General");
}

using Tellico::Data::FileCatalog;

FileCatalog::FileCatalog(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_.isEmpty() ? i18n("My Files") : title_) {
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("volume"));
}

Tellico::Data::FieldVec FileCatalog::defaultFields() {
  FieldVec list;
  FieldPtr field;

  field = new Field(QString::fromLatin1("title"), i18n("Name"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::NoDelete);
  list.append(field);

  field = new Field(QString::fromLatin1("url"), i18n("URL"), Field::URL);
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QString::fromLatin1("description"), i18n("Description"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("volume"), i18n("Volume"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("folder"), i18n("Folder"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("mimetype"), i18n("Mimetype"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("size"), i18n("Size"));
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QString::fromLatin1("permissions"), i18n("Permissions"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("owner"), i18n("Owner"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("group"), i18n("Group"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  // these dates are string fields, not dates, since the time is included
  field = new Field(QString::fromLatin1("created"), i18n("Created"));
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QString::fromLatin1("modified"), i18n("Modified"));
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QString::fromLatin1("metainfo"), i18n("Meta Info"), Field::Table);
  field->setProperty(QString::fromLatin1("columns"), QChar('2'));
  field->setProperty(QString::fromLatin1("column1"), i18n("Property"));
  field->setProperty(QString::fromLatin1("column2"), i18n("Value"));
  list.append(field);

  field = new Field(QString::fromLatin1("icon"), i18n("Icon"), Field::Image);
  list.append(field);

  return list;
}

int FileCatalog::sameEntry(Data::EntryPtr entry1_, Data::EntryPtr entry2_) const {
  // equal urls are always equal, even if modification time or something is different
  if(Entry::compareValues(entry1_, entry2_, QString::fromLatin1("url"), this) > 0) {
    return 100; // good match
  }
  // if volume or created time is different, it can't be same entry
  if(Entry::compareValues(entry1_, entry2_, QString::fromLatin1("volume"), this) == 0 ||
     Entry::compareValues(entry1_, entry2_, QString::fromLatin1("created"), this) == 0 ||
     Entry::compareValues(entry1_, entry2_, QString::fromLatin1("size"), this) == 0) {
    return 0;
  }
  int res = Entry::compareValues(entry1_, entry2_, QString::fromLatin1("title"), this);
  res += Entry::compareValues(entry1_, entry2_, QString::fromLatin1("description"), this);
  res += Entry::compareValues(entry1_, entry2_, QString::fromLatin1("mimetype"), this);
  return res;
}

#include "filecatalog.moc"
