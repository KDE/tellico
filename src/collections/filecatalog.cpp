/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "filecatalog.h"
#include "../entrycomparison.h"

#include <KLocalizedString>

namespace {
  static const char* file_general = I18N_NOOP("General");
}

using Tellico::Data::FileCatalog;

FileCatalog::FileCatalog(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Files") : title_) {
  setDefaultGroupField(QStringLiteral("volume"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList FileCatalog::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = Field::createDefaultField(Field::TitleField);
  field->setTitle(i18n("Name"));
  field->setFormatType(FieldFormat::FormatNone);
  list.append(field);

  field = new Field(QStringLiteral("url"), i18n("URL"), Field::URL);
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QStringLiteral("description"), i18n("Description"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("volume"), i18nc("File catalog", "Volume"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("folder"), i18n("Folder"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("mimetype"), i18n("Mimetype"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("size"), i18n("Size"));
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QStringLiteral("permissions"), i18n("Permissions"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("owner"), i18n("Owner"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("group"), i18n("Group"));
  field->setCategory(i18n(file_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  // these dates are string fields, not dates, since the time is included
  field = new Field(QStringLiteral("created"), i18n("Created"));
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QStringLiteral("modified"), i18n("Modified"));
  field->setCategory(i18n(file_general));
  list.append(field);

  field = new Field(QStringLiteral("metainfo"), i18n("Meta Info"), Field::Table);
  field->setProperty(QStringLiteral("columns"), QStringLiteral("2"));
  field->setProperty(QStringLiteral("column1"), i18n("Property"));
  field->setProperty(QStringLiteral("column2"), i18n("Value"));
  list.append(field);

  field = new Field(QStringLiteral("icon"), i18n("Icon"), Field::Image);
  list.append(field);

  return list;
}

int FileCatalog::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  // equal urls are always equal, even if modification time or something is different
  if(EntryComparison::score(entry1_, entry2_, QStringLiteral("url"), this) > 0) {
    return 100; // good match
  }
  // if volume or created time is different, it can't be same entry
  if(EntryComparison::score(entry1_, entry2_, QStringLiteral("volume"), this) == 0 ||
     EntryComparison::score(entry1_, entry2_, QStringLiteral("created"), this) == 0 ||
     EntryComparison::score(entry1_, entry2_, QStringLiteral("size"), this) == 0) {
    return 0;
  }
  int res = EntryComparison::score(entry1_, entry2_, QStringLiteral("title"), this);
  res += EntryComparison::score(entry1_, entry2_, QStringLiteral("description"), this);
  res += EntryComparison::score(entry1_, entry2_, QStringLiteral("mimetype"), this);
  return res;
}
