/***************************************************************************
    copyright            : (C) 2007-2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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
  Data::FieldPtr m_field;
};

class ValueComparison : public FieldComparison {
public:
  ValueComparison(Data::FieldPtr field, StringComparison* comp);
  ~ValueComparison();

  using FieldComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2);

private:
  StringComparison* m_stringComparison;
};

class ImageComparison : public FieldComparison {
public:
  ImageComparison(Data::FieldPtr field);

  using FieldComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2);
};

class DependentComparison : public FieldComparison {
public:
  DependentComparison(Data::FieldPtr field);
  ~DependentComparison();

  virtual int compare(Data::EntryPtr entry1, Data::EntryPtr entry2);

  using FieldComparison::compare;

protected:
  virtual int compare(const QString&, const QString&) { return 0; }

private:
  QList<FieldComparison*> m_comparisons;
};

class ChoiceComparison : public FieldComparison {
public:
  ChoiceComparison(Data::FieldPtr field);

  using FieldComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2);

private:
  QStringList m_values;
};

}
#endif
