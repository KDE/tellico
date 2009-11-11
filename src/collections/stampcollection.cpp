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

#include "stampcollection.h"

#include <klocale.h>

namespace {
  static const char* stamp_general = I18N_NOOP("General");
  static const char* stamp_condition = I18N_NOOP("Condition");
  static const char* stamp_personal = I18N_NOOP("Personal");
}

using Tellico::Data::StampCollection;

StampCollection::StampCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("My Stamps") : title_) {
  setDefaultGroupField(QLatin1String("denomination"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }
}

Tellico::Data::FieldList StampCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  field = createDefaultField(TitleField);
  field->setProperty(QLatin1String("template"), QLatin1String("%{year} %{description} %{denomination}"));
  field->setFlags(Field::NoDelete | Field::Derived);
  list.append(field);

  field = new Field(QLatin1String("description"), i18n("Description"));
  field->setCategory(i18n(stamp_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QLatin1String("denomination"), i18n("Denomination"));
  field->setCategory(i18n(stamp_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("country"), i18n("Country"));
  field->setCategory(i18n(stamp_general));
  field->setFormatType(FieldFormat::FormatPlain);
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("year"), i18n("Issue Year"), Field::Number);
  field->setCategory(i18n(stamp_general));
  field->setFlags(Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("color"), i18n("Color"));
  field->setCategory(i18n(stamp_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("scott"), i18n("Scott#"));
  field->setCategory(i18n(stamp_general));
  list.append(field);

  QStringList grade = i18nc("Stamp grade levels - "
                            "Superb,Extremely Fine,Very Fine,Fine,Average,Poor",
                            "Superb,Extremely Fine,Very Fine,Fine,Average,Poor")
                      .split(QRegExp(QLatin1String("\\s*,\\s*")), QString::SkipEmptyParts);
  field = new Field(QLatin1String("grade"), i18n("Grade"), grade);
  field->setCategory(i18n(stamp_condition));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("cancelled"), i18n("Cancelled"), Field::Bool);
  field->setCategory(i18n(stamp_condition));
  list.append(field);

  /* TRANSLATORS: See http://en.wikipedia.org/wiki/Stamp_hinge */
  field = new Field(QLatin1String("hinged"), i18n("Hinged"));
  field->setCategory(i18n(stamp_condition));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("centering"), i18n("Centering"));
  field->setCategory(i18n(stamp_condition));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("gummed"), i18n("Gummed"));
  field->setCategory(i18n(stamp_condition));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("pur_date"), i18n("Purchase Date"));
  field->setCategory(i18n(stamp_personal));
  field->setFormatType(FieldFormat::FormatDate);
  list.append(field);

  field = new Field(QLatin1String("pur_price"), i18n("Purchase Price"));
  field->setCategory(i18n(stamp_personal));
  list.append(field);

  field = new Field(QLatin1String("location"), i18n("Location"));
  field->setCategory(i18n(stamp_personal));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("gift"), i18n("Gift"), Field::Bool);
  field->setCategory(i18n(stamp_personal));
  list.append(field);

  field = new Field(QLatin1String("image"), i18n("Image"), Field::Image);
  list.append(field);

  field = new Field(QLatin1String("comments"), i18n("Comments"), Field::Para);
  list.append(field);

  list.append(createDefaultField(IDField));
  list.append(createDefaultField(CreatedDateField));
  list.append(createDefaultField(ModifiedDateField));

  return list;
}

#include "stampcollection.moc"
