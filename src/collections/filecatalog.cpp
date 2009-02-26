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
#include "../entrycomparison.h"

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
  setDefaultGroupField(QLatin1String("volume"));
}

Tellico::Data::FieldList FileCatalog::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = new Field(QLatin1String("title"), i18n("Name"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::NoDelete);
  list.append(field);

  field = new Field(QLatin1String("url"), i18n("URL"), Field::URL);
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QLatin1String("description"), i18n("Description"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("volume"), i18n("Volume"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("folder"), i18n("Folder"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("mimetype"), i18n("Mimetype"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("size"), i18n("Size"));
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QLatin1String("permissions"), i18n("Permissions"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("owner"), i18n("Owner"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("group"), i18n("Group"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  // these dates are string fields, not dates, since the time is included
  field = new Field(QLatin1String("created"), i18n("Created"));
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QLatin1String("modified"), i18n("Modified"));
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QLatin1String("metainfo"), i18n("Meta Info"), Field::Table);
  field->setProperty(QLatin1String("columns"), QLatin1String("2"));
  field->setProperty(QLatin1String("column1"), i18n("Property"));
  field->setProperty(QLatin1String("column2"), i18n("Value"));
  list.append(field);

  field = new Field(QLatin1String("icon"), i18n("Icon"), Field::Image);
  list.append(field);

  return list;
}

int FileCatalog::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  // equal urls are always equal, even if modification time or something is different
  if(EntryComparison::score(entry1_, entry2_, QLatin1String("url"), this) > 0) {
    return 100; // good match
  }
  // if volume or created time is different, it can't be same entry
  if(EntryComparison::score(entry1_, entry2_, QLatin1String("volume"), this) == 0 ||
     EntryComparison::score(entry1_, entry2_, QLatin1String("created"), this) == 0 ||
     EntryComparison::score(entry1_, entry2_, QLatin1String("size"), this) == 0) {
    return 0;
  }
  int res = EntryComparison::score(entry1_, entry2_, QLatin1String("title"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("description"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("mimetype"), this);
  return res;
}

#include "filecatalog.moc"
