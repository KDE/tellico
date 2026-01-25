/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "fieldcomparison.h"
#include "stringcomparison.h"
#include "../field.h"
#include "../images/imagefactory.h"
#include "../images/image.h"

#include <QDateTime>

using namespace Tellico;

std::unique_ptr<Tellico::FieldComparison> Tellico::FieldComparison::create(Data::FieldPtr field_) {
  if(!field_) {
    return nullptr;
  }
  FieldComparison* comp = nullptr;
  if(field_->type() == Data::Field::Image) {
    comp = new ImageComparison(field_);
  } else if(field_->type() == Data::Field::Choice) {
    comp = new ChoiceComparison(field_);
  } else {
    comp = new ValueComparison(field_, StringComparison::create(field_));
  }
  return std::unique_ptr<FieldComparison>(comp);
}

Tellico::FieldComparison::FieldComparison(Data::FieldPtr field_) : m_field(field_) {
}

int Tellico::FieldComparison::compare(Data::EntryPtr entry1_, Data::EntryPtr entry2_) {
  return compare(entry1_->formattedField(m_field), entry2_->formattedField(m_field));
}

Tellico::ValueComparison::ValueComparison(Data::FieldPtr field, std::unique_ptr<StringComparison> comp)
    : FieldComparison(field)
    , m_stringComparison(std::move(comp)) {
  Q_ASSERT(m_stringComparison);
}

Tellico::ValueComparison::~ValueComparison() = default;

int Tellico::ValueComparison::compare(const QString& str1_, const QString& str2_) {
  return m_stringComparison->compare(str1_, str2_);
}

Tellico::ImageComparison::ImageComparison(Data::FieldPtr field) : FieldComparison(field) {
}

int Tellico::ImageComparison::compare(const QString& str1_, const QString& str2_) {
  if(str1_.isEmpty()) {
    if(str2_.isEmpty()) {
      return 0;
    }
    return -1;
  }
  if(str2_.isEmpty()) {
    return 1;
  }

  const Data::Image& image1 = ImageFactory::imageById(str1_);
  const Data::Image& image2 = ImageFactory::imageById(str2_);
  if(image1.isNull()) {
    if(image2.isNull()) {
      return 0;
    }
    return -1;
  }
  if(image2.isNull()) {
    return 1;
  }
  // large images come first
  return image1.width() - image2.width();
}

Tellico::ChoiceComparison::ChoiceComparison(Data::FieldPtr field) : FieldComparison(field) {
  m_values = field->allowed();
}

int Tellico::ChoiceComparison::compare(const QString& str1, const QString& str2) {
  return m_values.indexOf(str1) - m_values.indexOf(str2);
}
