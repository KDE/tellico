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

// this constructor is for anything but Choice type
Field::Field(const QString& name_, const QString& title_, Type type_/*=Line*/)
    : QSharedData(), m_name(Tellico::shareString(name_)), m_title(title_),  m_category(i18n("General")), m_desc(title_),
      m_type(type_), m_flags(0), m_formatType(FieldFormat::FormatNone) {

  Q_ASSERT(m_type != Choice);
  // a paragraph's category is always its title, along with tables
  if(isSingleCategory()) {
    m_category = m_title;
  }
  switch(m_type) {
    case Table:
    case Table2:
      m_flags = AllowMultiple;
      if(m_type == Table2) {
        m_type = Table;
        setProperty(QStringLiteral("columns"), QStringLiteral("2"));
      } else {
        setProperty(QStringLiteral("columns"), QStringLiteral("1"));
      }
      break;
    case Date:  // hidden from user
      m_formatType = FieldFormat::FormatDate;
      break;
    case Rating:
      setProperty(QStringLiteral("minimum"), QStringLiteral("1"));
      setProperty(QStringLiteral("maximum"), QStringLiteral("5"));
      break;
    case ReadOnly:
      m_flags = NoEdit;
      m_type = Line;
      break;
    case Dependent:
      m_flags = Derived;
      m_type = Line;
      break;
    default: // ssshhhhhhhh
      break;
  }
}

// if this constructor is called, the type is necessarily Choice
Field::Field(const QString& name_, const QString& title_, const QStringList& allowed_)
    : QSharedData(), m_name(Tellico::shareString(name_)), m_title(title_), m_category(i18n("General")), m_desc(title_),
      m_type(Field::Choice), m_allowed(allowed_), m_flags(0), m_formatType(FieldFormat::FormatNone) {
}

Field::Field(const Field& field_)
    : QSharedData(field_), m_name(field_.name()), m_title(field_.title()), m_category(field_.category()),
      m_desc(field_.description()), m_type(field_.type()), m_allowed(field_.allowed()),
      m_flags(field_.flags()), m_formatType(field_.formatType()),
      m_properties(field_.propertyList()) {
}

Field& Field::operator=(const Field& field_) {
  if(this == &field_) return *this;

//  static_cast<QSharedData&>(*this) = static_cast<const QSharedData&>(field_);
  m_name = field_.name();
  m_title = field_.title();
  m_category = field_.category();
  m_desc = field_.description();
  m_type = field_.type();
  m_allowed = field_.allowed();
  m_flags = field_.flags();
  m_formatType = field_.formatType();
  m_properties = field_.propertyList();
  return *this;
}

Field::~Field() = default;

void Field::setTitle(const QString& title_) {
  m_title = title_;
  if(isSingleCategory()) {
    m_category = title_;
  }
}

void Field::setType(Field::Type type_) {
  m_type = type_;
  if(m_type != Field::Choice) {
    m_allowed = QStringList();
  }
  switch(m_type) {
    case Table:
    case Table2:
      m_flags |= AllowMultiple;
      if(m_type == Table2) {
        m_type = Table;
        setProperty(QStringLiteral("columns"), QStringLiteral("2"));
      }
      if(property(QStringLiteral("columns")).isEmpty()) {
        setProperty(QStringLiteral("columns"), QStringLiteral("1"));
      }
      break;
    case Date:
      m_formatType = FieldFormat::FormatDate;
      break;
    case ReadOnly:
      m_flags |= NoEdit;
      m_type = Line;
      break;
    case Dependent:
      m_flags |= Derived;
      m_type = Line;
      break;
    default: // ssshhhhhhhh
      break;
  }
  if(isSingleCategory()) {
    m_category = m_title;
  }
}

void Field::setCategory(const QString& category_) {
  if(!isSingleCategory()) {
    m_category = category_;
  }
}

void Field::setFlags(int flags_) {
  // tables always have multiple allowed
  if(m_type == Table) {
    m_flags = AllowMultiple | flags_;
  } else {
    m_flags = flags_;
  }
}

bool Field::hasFlag(FieldFlag flag_) const {
  return m_flags & flag_;
}

void Field::setFormatType(FieldFormat::Type type_) {
  // Choice and Data fields are not allowed a format type
  if(m_type != Choice && m_type != Date) {
    m_formatType = type_;
  }
}

QString Field::defaultValue() const {
  return property(QStringLiteral("default"));
}

void Field::setDefaultValue(const QString& value_) {
  if(value_.isEmpty() || m_type != Choice || m_allowed.contains(value_)) {
    setProperty(QStringLiteral("default"), value_);
  }
}

bool Field::isSingleCategory() const {
  return (m_type == Para || m_type == Table || m_type == Image);
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

void Field::addAllowed(const QString& value_) {
  if(m_type != Choice) {
    return;
  }
  if(!m_allowed.contains(value_)) {
    m_allowed += value_;
  }
}

void Field::setProperty(const QString& key_, const QString& value_) {
  if(value_.isEmpty()) {
    m_properties.remove(key_);
  } else {
    m_properties.insert(key_, value_);
  }
}

void Field::setPropertyList(const Tellico::StringMap& props_) {
  m_properties = props_;
}

QString Field::property(const QString& key_) const {
  return m_properties.value(key_);
}

void Field::convertOldRating(Tellico::Data::FieldPtr field_) {
  if(field_->type() != Data::Field::Choice) {
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
  Data::FieldPtr field;
  switch(fieldEnum) {
    case IDField:
      field = new Field(QStringLiteral("id"), i18nc("ID # of the entry", "ID"), Field::Number);
      field->setCategory(i18n("Personal"));
      field->setProperty(QStringLiteral("template"), QStringLiteral("%{@id}"));
      field->setFlags(Field::Derived);
      field->setFormatType(FieldFormat::FormatNone);
      break;
    case TitleField:
      field = new Field(QStringLiteral("title"), i18n("Title"));
      field->setCategory(i18n("General"));
      field->setFlags(Field::NoDelete);
      field->setFormatType(FieldFormat::FormatTitle);
      break;
    case CreatedDateField:
      field = new Field(QStringLiteral("cdate"), i18n("Date Created"), Field::Date);
      field->setCategory(i18n("Personal"));
      field->setFlags(Field::NoEdit);
      break;
    case ModifiedDateField:
      field = new Field(QStringLiteral("mdate"), i18n("Date Modified"), Field::Date);
      field->setCategory(i18n("Personal"));
      field->setFlags(Field::NoEdit);
      break;
    case IsbnField:
      field = new Field(QStringLiteral("isbn"), i18n("ISBN#"));
      field->setCategory(i18n("Publishing"));
      field->setDescription(i18n("International Standard Book Number"));
      break;
    case LccnField:
      field = new Field(QStringLiteral("lccn"), i18n("LCCN#"));
      field->setCategory(i18n("Publishing"));
      field->setDescription(i18n("Library of Congress Control Number"));
      break;
    case PegiField:
      {
      QStringList pegi = QStringLiteral("PEGI 3, PEGI 7, PEGI 12, PEGI 16, PEGI 18")
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
                         .split(FieldFormat::commaSplitRegularExpression(), QString::SkipEmptyParts);
#else
                         .split(FieldFormat::commaSplitRegularExpression(), Qt::SkipEmptyParts);
#endif
      field = new Field(QStringLiteral("pegi"), i18n("PEGI Rating"), pegi);
      }
      field->setCategory(i18n("General"));
      field->setFlags(Field::AllowGrouped);
      break;
    case ImdbField:
      field = new Field(QStringLiteral("imdb"), i18n("IMDb Link"), Field::URL);
      field->setCategory(i18n("General"));
      break;
    case EpisodeField:
      field = new Data::Field(QStringLiteral("episode"), i18n("Episodes"), Data::Field::Table);
      field->setFormatType(FieldFormat::FormatTitle);
      field->setProperty(QStringLiteral("columns"), QStringLiteral("3"));
      field->setProperty(QStringLiteral("column1"), i18n("Title"));
      field->setProperty(QStringLiteral("column2"), i18nc("TV Season", "Season"));
      field->setProperty(QStringLiteral("column3"), i18nc("TV Episode", "Episode"));
      break;
    case ScreenshotField:
      field = new Field(QStringLiteral("screenshot"), i18n("Screenshot"), Field::Image);
      break;
    case FrontCoverField:
      field = new Field(QStringLiteral("cover"), i18n("Front Cover"), Field::Image);
      break;      
  }
  Q_ASSERT(field);
  return field;
}

Tellico::Data::FieldList Tellico::listIntersection(const Tellico::Data::FieldList& list1, const Tellico::Data::FieldList& list2) {
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
  // QList::toSet is deprecated
  return list1.toSet().intersect(list2.toSet()).values();
#else
  // std::set_intersection requires sorted lists. Just do the set intersection manually
  Data::FieldList returnList;
  QSet<Tellico::Data::FieldPtr> set(list1.begin(), list1.end());

  std::for_each(list2.begin(), list2.end(),
        [&set, &returnList](const Data::FieldPtr& f) {
          if(set.contains(f)) {
            returnList.append(f);
            set.remove(f);
          }
        }
      );
  return returnList;
#endif
}
