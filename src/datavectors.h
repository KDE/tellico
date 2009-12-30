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

#ifndef TELLICO_DATA_VECTORS_H
#define TELLICO_DATA_VECTORS_H

#include <QList>
#include <QVector>
#include <QMap>
#include <QHash>
#include <QPair>
#include <QMetaType>

#include <ksharedptr.h>

namespace Tellico {
  typedef QMap<QString, QString> StringMap;
  typedef QHash<QString, QString> StringHash;

  class Filter;
  typedef KSharedPtr<Filter> FilterPtr;
  typedef QList<FilterPtr> FilterList;

  namespace Data {
    // used for fields, collections, and entries
    typedef int ID;

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
