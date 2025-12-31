/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_COLLECTIONFACTORY_H
#define TELLICO_COLLECTIONFACTORY_H

#include "datavectors.h"

#include <QHash>

namespace Tellico {

typedef QHash<int, QString> CollectionNameHash;

/**
 * A factory class for dealing with the different types of collections.
 *
 * @author Robby Stephenson
 */
class CollectionFactory {

typedef Data::CollPtr (*CREATE_COLL_FN)(bool);

public:
  /** Singleton access.
   */
  static CollectionFactory& self();
  static Data::CollPtr collection(int type, bool addDefaultFields);
  static Data::CollPtr collection(const QString& typeName, bool addDefaultFields);
  static CollectionNameHash nameHash();
  static QString typeName(int type);
  static QString typeName(Data::CollPtr coll);
  static bool isDefaultField(int type, const QString& name);

  // public so we can iterate over them
  typedef QHash<int, QString> TypeStringHash;
  TypeStringHash nameRegistry;

  /**
   * Classes derived from Collection call this function once
   * per program to register the class ID key, and a pointer to
   * the function that creates the class.
   */
  void registerFunction(int type, const QString& typeName, CREATE_COLL_FN func);

  /**
   * Create a new collection of type
   *
   * @param addDefaultFields add the default fields to the collection
   */
  Data::CollPtr create(int type, bool addDefaultFields) const;

private:
  CollectionFactory();
  ~CollectionFactory() = default;
  Q_DISABLE_COPY(CollectionFactory)

  /**
   * Keep a hash of all the function pointers to create classes
   */
  typedef QHash<int, CREATE_COLL_FN> FunctionRegistry;
  FunctionRegistry functionRegistry;
};

/**
 * Helper template for registering collection classes
 */
template <class Derived>
class RegisterCollection {
public:
  static Tellico::Data::CollPtr createInstance(bool addDefaultFields) {
    return Tellico::Data::CollPtr(new Derived(addDefaultFields));
  }
  RegisterCollection(int type, const char* typeName) {
    CollectionFactory::self().registerFunction(type, QLatin1String(typeName), createInstance);
  }
};

} // end namespace
#endif
