/***************************************************************************
    copyright            : (C) 2007 by Robby Stephenson
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

#include <qregexp.h>

class QStringList;
class QIconViewItem;

namespace Tellico {
  namespace GUI {
    class ListViewItem;
  }

class ListViewComparison {
public:
  ListViewComparison(Data::ConstFieldPtr field);
  virtual ~ListViewComparison() {}

  const QString& fieldName() const { return m_fieldName; }

  virtual int compare(int col, const GUI::ListViewItem* item1, const GUI::ListViewItem* item2, bool asc);
  virtual int compare(const QIconViewItem* item1, const QIconViewItem* item2);

  static ListViewComparison* create(Data::FieldPtr field);
  static ListViewComparison* create(Data::ConstFieldPtr field);

protected:
  virtual int compare(const QString& str1, const QString& str2) = 0;

private:
  QString m_fieldName;
};

class StringComparison : public ListViewComparison {
public:
  StringComparison(Data::ConstFieldPtr field);

protected:
  virtual int compare(const QString& str1, const QString& str2);
};

class TitleComparison : public ListViewComparison {
public:
  TitleComparison(Data::ConstFieldPtr field);

protected:
  virtual int compare(const QString& str1, const QString& str2);
};

class NumberComparison : public ListViewComparison {
public:
  NumberComparison(Data::ConstFieldPtr field);

protected:
  virtual int compare(const QString& str1, const QString& str2);
};

class LCCComparison : public StringComparison {
public:
  LCCComparison(Data::ConstFieldPtr field);

protected:
  virtual int compare(const QString& str1, const QString& str2);

private:
  int compareLCC(const QStringList& cap1, const QStringList& cap2) const;
  QRegExp m_regexp;
};

class PixmapComparison : public ListViewComparison {
public:
  PixmapComparison(Data::ConstFieldPtr field);

  virtual int compare(int col, const GUI::ListViewItem* item1, const GUI::ListViewItem* item2, bool asc);
  virtual int compare(const QIconViewItem* item1, const QIconViewItem* item2);

protected:
  virtual int compare(const QString&, const QString&) { return 0; }
};

class DependentComparison : public StringComparison {
public:
  DependentComparison(Data::ConstFieldPtr field);

  virtual int compare(int col, const GUI::ListViewItem* item1, const GUI::ListViewItem* item2, bool asc);
  virtual int compare(const QIconViewItem* item1, const QIconViewItem* item2);

private:
  QPtrList<ListViewComparison> m_comparisons;
};

class ISODateComparison : public ListViewComparison {
public:
  ISODateComparison(Data::ConstFieldPtr field);

protected:
  virtual int compare(const QString& str1, const QString& str2);
};

class ChoiceComparison : public ListViewComparison {
public:
  ChoiceComparison(Data::ConstFieldPtr field);

protected:
  virtual int compare(const QString& str1, const QString& str2);

private:
  QStringList m_values;
};

}
#endif
