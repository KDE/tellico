/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef DATA_VECTORS_H
#define DATA_VECTORS_H

#include "ptrvector.h"

#include <qmap.h>
#include <qpair.h>

#include <ksharedptr.h>

namespace Tellico {
  typedef QMap<QString, QString> StringMap;

  class Filter;
  typedef KSharedPtr<Filter> FilterPtr;
  typedef Vector<Filter> FilterVec;

  namespace Data {
    class Collection;
    typedef KSharedPtr<Collection> CollPtr;
    typedef Vector<Collection> CollVec;

    class Field;
    typedef KSharedPtr<Field> FieldPtr;
    typedef KSharedPtr<const Field> ConstFieldPtr;
    typedef Vector<Field> FieldVec;
//    typedef Vector<ConstFieldPtr> ConstFieldVec;

    class Entry;
    typedef KSharedPtr<Entry> EntryPtr;
    typedef KSharedPtr<const Entry> ConstEntryPtr;
    typedef Vector<Entry> EntryVec;
    typedef EntryVec::Iterator EntryVecIt;
    typedef Vector<const Entry> ConstEntryVec;
    // complicated, I know
    // first item is a vector of all entries that got added in the merge process
    // second item is a pair of entries that had their track field modified
    // since a music collection is the only one that would actually merge entries
    typedef QValueVector< QPair<EntryPtr, QString> > PairVector;
    typedef QPair<Data::EntryVec, PairVector> MergePair;

    class Borrower;
    typedef KSharedPtr<Borrower> BorrowerPtr;
    typedef Vector<Borrower> BorrowerVec;
  }
}

#endif
