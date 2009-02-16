/***************************************************************************
    copyright            : (C) 2001-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_DATA_VECTORS_H
#define TELLICO_DATA_VECTORS_H

#include <QList>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QMetaType>

#include <ksharedptr.h>

namespace Tellico {
  typedef QMap<QString, QString> StringMap;

  class Filter;
  typedef KSharedPtr<Filter> FilterPtr;
  typedef QList<FilterPtr> FilterList;

  namespace Data {
    class Collection;
    typedef KSharedPtr<Collection> CollPtr;
    typedef QList<CollPtr> CollList;

    class Field;
    typedef KSharedPtr<Field> FieldPtr;
    typedef QList<FieldPtr> FieldList;

    class Entry;
    typedef KSharedPtr<Entry> EntryPtr;
    typedef QList<EntryPtr> EntryList;

    // complicated, I know
    // first item is a vector of all entries that got added in the merge process
    // second item is a pair of entries that had their track field modified
    // since a music collection is the only one that would actually merge entries
    typedef QVector< QPair<EntryPtr, QString> > PairVector;
    typedef QPair<Data::EntryList, PairVector> MergePair;

    class Borrower;
    typedef KSharedPtr<Borrower> BorrowerPtr;
    typedef QList<BorrowerPtr> BorrowerList;
  }
}

#endif
