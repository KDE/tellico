/***************************************************************************
    copyright            : (C) 2007-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_LISTVIEWCOMPARISON_H
#define TELLICO_LISTVIEWCOMPARISON_H

#include "datavectors.h"

#include <QRegExp>
#include <QList>
#include <QStringList>

namespace Tellico {

class ListViewComparison {
public:
  ListViewComparison(Data::FieldPtr field);
  virtual ~ListViewComparison() {}

  Data::FieldPtr field() const { return m_field; }

  virtual int compare(Data::EntryPtr entry1, Data::EntryPtr entry2);

  static ListViewComparison* create(Data::FieldPtr field);

protected:
  virtual int compare(const QString& str1, const QString& str2) = 0;

private:
  Data::FieldPtr m_field;
};

class BoolComparison : public ListViewComparison {
public:
  BoolComparison(Data::FieldPtr field);

  using ListViewComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2);
};

class StringComparison : public ListViewComparison {
public:
  StringComparison(Data::FieldPtr field);

  using ListViewComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2);
};

class TitleComparison : public ListViewComparison {
public:
  TitleComparison(Data::FieldPtr field);

  using ListViewComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2);
};

class NumberComparison : public ListViewComparison {
public:
  NumberComparison(Data::FieldPtr field);

  using ListViewComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2);
};

class LCCComparison : public StringComparison {
public:
  LCCComparison(Data::FieldPtr field);

  using ListViewComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2);

private:
  int compareLCC(const QStringList& cap1, const QStringList& cap2) const;
  QRegExp m_regexp;
};

class ImageComparison : public ListViewComparison {
public:
  ImageComparison(Data::FieldPtr field);

  virtual int compare(Data::EntryPtr entry1, Data::EntryPtr entry2);

  using ListViewComparison::compare;

protected:
  virtual int compare(const QString&, const QString&) { return 0; }
};

class RatingComparison : public ListViewComparison {
public:
  RatingComparison(Data::FieldPtr field);

  using ListViewComparison::compare;

protected:
  virtual int compare(const QString&, const QString&);
};

class DependentComparison : public StringComparison {
public:
  DependentComparison(Data::FieldPtr field);
  ~DependentComparison();

  virtual int compare(Data::EntryPtr entry1, Data::EntryPtr entry2);

  using ListViewComparison::compare;

private:
  QList<ListViewComparison*> m_comparisons;
};

class ISODateComparison : public ListViewComparison {
public:
  ISODateComparison(Data::FieldPtr field);

  using ListViewComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2);
};

class ChoiceComparison : public ListViewComparison {
public:
  ChoiceComparison(Data::FieldPtr field);

  using ListViewComparison::compare;

protected:
  virtual int compare(const QString& str1, const QString& str2);

private:
  QStringList m_values;
};

}
#endif
