/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#include "field.h"
#include "fieldformat.h"
#include "utils/string_utils.h"

#include <KLocalizedString>

#include <algorithm>

using namespace Tellico;
using Tellico::Data::Field;

class Q_DECL_HIDDEN Field::Data : public QSharedData {
public:
  QString name;
  QString title;
  QString category;
  QString desc;
  Type type;
  QStringList allowed;
  int flags = 0;
  FieldFormat::Type formatType = FieldFormat::FormatNone;
  QMap<QString, QString> properties;

  Data() {}
};

// this constructor is for anything but Choice type
Field::Field(const QString& name_, const QString& title_, Type type_/*=Line*/)
    : d(new Data) {
  d->name = Tellico::shareString(name_);
  d->title = title_;
  d->category = i18n("General");
  d->desc = title_;
  d->type = type_;

  Q_ASSERT(d->type != Choice);
  // a paragraph's category is always its title, along with tables
  if(isSingleCategory()) {
    d->category = d->title;
  }
  switch(d->type) {
    case Table:
    case Table2:
      d->flags = AllowMultiple;
      if(d->type == Table2) {
        d->type = Table;
        setProperty(QStringLiteral("columns"), QStringLiteral("2"));
      } else {
        setProperty(QStringLiteral("columns"), QStringLiteral("1"));
      }
      break;
    case Date:  // hidden from user
      d->formatType = FieldFormat::FormatDate;
      break;
    case Rating:
      setProperty(QStringLiteral("minimum"), QStringLiteral("1"));
      setProperty(QStringLiteral("maximum"), QStringLiteral("5"));
      break;
    case ReadOnly:
      d->flags = NoEdit;
      d->type = Line;
      break;
    case Dependent:
      d->flags = Derived;
      d->type = Line;
      break;
    default: // ssshhhhhhhh
      break;
  }
}

// if this constructor is called, the type is necessarily Choice
Field::Field(const QString& name_, const QString& title_, const QStringList& allowed_)
    : d(new Data) {
  d->name = Tellico::shareString(name_);
  d->title = title_;
  d->category = i18n("General");
  d->desc = title_;
  d->type = Field::Choice;
  d->allowed = allowed_;
}

Field::Field(const Field& field_)
    : d(field_.d) {
}

Field& Field::operator=(const Field& field_) {
  if(this == &field_) return *this;

  d = field_.d;
  return *this;
}

Field::~Field() = default;

QString Field::name() const {
  return d->name;
}

void Field::setName(const QString& name_) {
  d->name = name_;
}

QString Field::title() const {
  return d->title;
}

void Field::setTitle(const QString& title_) {
  d->title = title_;
  if(isSingleCategory()) {
    d->category = title_;
  }
}

Tellico::Data::Field::Type Field::type() const {
  return d->type;
}

void Field::setType(Field::Type type_) {
  d->type = type_;
  if(d->type != Field::Choice) {
    d->allowed.clear();
  }
  switch(d->type) {
    case Table:
    case Table2:
      d->flags |= AllowMultiple;
      if(d->type == Table2) {
        d->type = Table;
        setProperty(QStringLiteral("columns"), QStringLiteral("2"));
      }
      if(property(QStringLiteral("columns")).isEmpty()) {
        setProperty(QStringLiteral("columns"), QStringLiteral("1"));
      }
      break;
    case Date:
      d->formatType = FieldFormat::FormatDate;
      break;
    case ReadOnly:
      d->flags |= NoEdit;
      d->type = Line;
      break;
    case Dependent:
      d->flags |= Derived;
      d->type = Line;
      break;
    default: // ssshhhhhhhh
      break;
  }
  if(isSingleCategory()) {
    d->category = d->title;
  }
}

int Field::flags() const {
  return d->flags;
}

QString Field::category() const {
  return d->category;
}

void Field::setCategory(const QString& category_) {
  if(!isSingleCategory()) {
    d->category = category_;
  }
}

void Field::setFlags(int flags_) {
  // tables always have multiple allowed
  if(d->type == Table) {
    d->flags = AllowMultiple | flags_;
  } else {
    d->flags = flags_;
  }
}

bool Field::hasFlag(FieldFlag flag_) const {
  return d->flags & flag_;
}

Tellico::FieldFormat::Type Field::formatType() const {
  return d->formatType;
}

void Field::setFormatType(FieldFormat::Type type_) {
  // Choice and Data fields are not allowed a format type
  if(d->type != Choice && d->type != Date) {
    d->formatType = type_;
  }
}

QString Field::description() const {
  return d->desc;
}

void Field::setDescription(const QString& desc_) {
  d->desc = desc_;
}

QString Field::defaultValue() const {
  return property(QStringLiteral("default"));
}

void Field::setDefaultValue(const QString& value_) {
  if(value_.isEmpty() || d->type != Choice || d->allowed.contains(value_)) {
    setProperty(QStringLiteral("default"), value_);
  }
}

bool Field::isSingleCategory() const {
  return (d->type == Para || d->type == Table || d->type == Image);
}

// if these are changed, then CollectionFieldsDialog should be checked since it
// checks for equality against some of these strings
Field::FieldMap Field::typeMap() {
  FieldMap map;
  map[Field::Line]      = i18n("Simple Text");
  map[Field::Para]      = i18n("Paragraph");
  map[Field::Choice]    = i18n("Choice");
  map[Field::Bool]      = i18n("Checkbox");
  map[Field::Number]    = i18n("Number");
  map[Field::URL]       = i18n("URL");
  map[Field::Table]     = i18n("Table");
  map[Field::Image]     = i18n("Image");
  map[Field::Date]      = i18n("Date");
  map[Field::Rating]    = i18n("Rating");
  return map;
}

// just for formatting's sake
QStringList Field::typeTitles() {
  const FieldMap& map = typeMap();
  QStringList list;
  list.append(map[Field::Line]);
  list.append(map[Field::Para]);
  list.append(map[Field::Choice]);
  list.append(map[Field::Bool]);
  list.append(map[Field::Number]);
  list.append(map[Field::URL]);
  list.append(map[Field::Date]);
  list.append(map[Field::Table]);
  list.append(map[Field::Image]);
  list.append(map[Field::Rating]);
  return list;
}

const QStringList& Field::allowed() const {
  return d->allowed;
}

void Field::setAllowed(const QStringList& allowed) {
  d->allowed = allowed;
}

void Field::addAllowed(const QString& value_) {
  if(d->type != Choice) {
    return;
  }
  if(!d->allowed.contains(value_)) {
    d->allowed += value_;
  }
}

void Field::setProperty(const QString& key_, const QString& value_) {
  if(value_.isEmpty()) {
    d->properties.remove(key_);
  } else {
    d->properties.insert(key_, value_);
  }
}

void Field::setPropertyList(const QMap<QString, QString>& props_) {
  d->properties = props_;
}

QString Field::property(const QString& key_) const {
  return d->properties.value(key_);
}

const QMap<QString, QString>& Field::propertyList() const {
  return d->properties;
}

void Field::convertOldRating(Tellico::Data::FieldPtr field_) {
  if(field_->type() != Choice) {
    return; // nothing to do
  }

  if(field_->name() != QLatin1String("rating")
     && field_->property(QStringLiteral("rating")) != QLatin1String("true")) {
    return; // nothing to do
  }

  int min = 10;
  int max = 1;
  bool ok;
  const QStringList& allow = field_->allowed();
  for(QStringList::ConstIterator it = allow.begin(); it != allow.end(); ++it) {
    int n = Tellico::toUInt(*it, &ok);
    if(!ok) {
      return; // no need to convert
    }
    min = qMin(min, n);
    max = qMax(max, n);
  }
  min = qMax(max, 1);
  max = qMin(max, 10);
  if(min >= max) {
    min = 1;
    max = 5;
  }
  field_->setProperty(QStringLiteral("minimum"), QString::number(min));
  field_->setProperty(QStringLiteral("maximum"), QString::number(max));
  //remove any old property
  field_->setProperty(QStringLiteral("rating"), QString());
  field_->setType(Rating);
}

Tellico::Data::FieldPtr Field::createDefaultField(DefaultField fieldEnum) {
  FieldPtr field;
  switch(fieldEnum) {
    case IDField:
      field.reset(new Field(QStringLiteral("id"), i18nc("ID # of the entry", "ID"), Number));
      field->setCategory(i18n("Personal"));
      field->setProperty(QStringLiteral("template"), QStringLiteral("%{@id}"));
      field->setFlags(Derived);
      field->setFormatType(FieldFormat::FormatNone);
      break;
    case TitleField:
      field.reset(new Field(QStringLiteral("title"), i18n("Title")));
      field->setCategory(i18n("General"));
      field->setFlags(NoDelete);
      field->setFormatType(FieldFormat::FormatTitle);
      break;
    case CreatedDateField:
      field.reset(new Field(QStringLiteral("cdate"), i18n("Date Created"), Date));
      field->setCategory(i18n("Personal"));
      field->setFlags(NoEdit);
      break;
    case ModifiedDateField:
      field.reset(new Field(QStringLiteral("mdate"), i18n("Date Modified"), Date));
      field->setCategory(i18n("Personal"));
      field->setFlags(NoEdit);
      break;
    case IsbnField:
      field.reset(new Field(QStringLiteral("isbn"), i18n("ISBN#")));
      field->setCategory(i18n("Publishing"));
      field->setDescription(i18n("International Standard Book Number"));
      break;
    case LccnField:
      field.reset(new Field(QStringLiteral("lccn"), i18n("LCCN#")));
      field->setCategory(i18n("Publishing"));
      field->setDescription(i18n("Library of Congress Control Number"));
      break;
    case PegiField:
      field.reset(new Field(QStringLiteral("pegi"),
                            i18n("PEGI Rating"),
                            {QStringLiteral("PEGI 3"),
                             QStringLiteral("PEGI 7"),
                             QStringLiteral("PEGI 12"),
                             QStringLiteral("PEGI 16"),
                             QStringLiteral("PEGI 18")}));
      field->setCategory(i18n("General"));
      field->setFlags(AllowGrouped);
      break;
    case ImdbField:
      field.reset(new Field(QStringLiteral("imdb"), i18n("IMDb Link"), URL));
      field->setCategory(i18n("General"));
      break;
    case EpisodeField:
      field.reset(new Field(QStringLiteral("episode"), i18n("Episodes"), Table));
      field->setFormatType(FieldFormat::FormatTitle);
      field->setProperty(QStringLiteral("columns"), QStringLiteral("3"));
      field->setProperty(QStringLiteral("column1"), i18n("Title"));
      field->setProperty(QStringLiteral("column2"), i18nc("TV Season", "Season"));
      field->setProperty(QStringLiteral("column3"), i18nc("TV Episode", "Episode"));
      break;
    case ScreenshotField:
      field.reset(new Field(QStringLiteral("screenshot"), i18n("Screenshot"), Image));
      break;
    case FrontCoverField:
      field.reset(new Field(QStringLiteral("cover"), i18n("Front Cover"), Image));
      break;
  }
  Q_ASSERT(field);
  return field;
}

Tellico::Data::FieldList Tellico::listIntersection(const Tellico::Data::FieldList& list1, const Tellico::Data::FieldList& list2) {
  // std::set_intersection requires sorted lists. Just do the set intersection manually
  Data::FieldList returnList;
  QSet<Tellico::Data::FieldPtr> set(list1.begin(), list1.end());

  std::for_each(list2.begin(), list2.end(),
        [&set, &returnList](const auto& f) {
          if(set.contains(f)) {
            returnList.append(f);
            set.remove(f);
          }
        }
      );
  return returnList;
}
