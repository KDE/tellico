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

class QListViewItem;
class QStringList;

#include "datavectors.h"

#include <qregexp.h>

namespace Tellico {

class ListViewComparison {
public:
  ListViewComparison() {}
  virtual ~ListViewComparison() {}

  virtual int compare(int col, const QListViewItem* item1, QListViewItem* item2, bool asc) = 0;

  static ListViewComparison* create(Data::FieldPtr field);
  static ListViewComparison* create(Data::ConstFieldPtr field);
};

class StringComparison : public ListViewComparison {
public:
  StringComparison() : ListViewComparison() {}

  virtual int compare(int col, const QListViewItem* item1, QListViewItem* item2, bool asc);
};

class TitleComparison : public ListViewComparison {
public:
  TitleComparison() : ListViewComparison() {}

  virtual int compare(int col, const QListViewItem* item1, QListViewItem* item2, bool asc);
};

class NumberComparison : public ListViewComparison {
public:
  NumberComparison() : ListViewComparison() {}

  virtual int compare(int col, const QListViewItem* item1, QListViewItem* item2, bool asc);
};

class LCCComparison : public StringComparison {
public:
  LCCComparison();

  virtual int compare(int col, const QListViewItem* item1, QListViewItem* item2, bool asc);

private:
  int compareLCC(const QStringList& cap1, const QStringList& cap2) const;
  QRegExp m_regexp;
};

class PixmapComparison : public ListViewComparison {
public:
  PixmapComparison() : ListViewComparison() {}

  virtual int compare(int col, const QListViewItem* item1, QListViewItem* item2, bool asc);
};

}
#endif
