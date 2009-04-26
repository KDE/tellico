/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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
  static bool isDefaultField(int type, const QString& name);

  // public so we can iterate over them
  typedef QHash<int, QString> TypeStringHash;
  TypeStringHash nameRegistry;
  TypeStringHash titleRegistry;

  /**
   * Classes derived from manufacturedObj call this function once
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
  /**
   * no copying
   */
  CollectionFactory();
  CollectionFactory(const CollectionFactory&); ///< Not implemented.
  CollectionFactory &operator=(const CollectionFactory&); ///< Not implemented.

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
