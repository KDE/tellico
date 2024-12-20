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

#ifndef TELLICO_FIELDCOMPARISON_H
#define TELLICO_FIELDCOMPARISON_H

#include "../datavectors.h"

#include <QStringList>

namespace Tellico {

class StringComparison;

class FieldComparison {
public:
  FieldComparison(Data::FieldPtr field);
  virtual ~FieldComparison() {}

  Data::FieldPtr field() const { return m_field; }

  virtual int compare(Data::EntryPtr entry1, Data::EntryPtr entry2);

  static FieldComparison* create(Data::FieldPtr field);

protected:
  virtual int compare(const QString& str1, const QString& str2) = 0;

private:
  Q_DISABLE_COPY(FieldComparison)
  Data::FieldPtr m_field;
};

class ValueComparison : public FieldComparison {
public:
  ValueComparison(Data::FieldPtr field, StringComparison* comp);
  ~ValueComparison();

  using FieldComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2) override;

private:
  StringComparison* m_stringComparison;
};

class ImageComparison : public FieldComparison {
public:
  ImageComparison(Data::FieldPtr field);

  using FieldComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2) override;
};

class ChoiceComparison : public FieldComparison {
public:
  ChoiceComparison(Data::FieldPtr field);

  using FieldComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2) override;

private:
  QStringList m_values;
};

}
#endif
