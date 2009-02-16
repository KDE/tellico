/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_CITE_OOOHANDLER_H
#define TELLICO_CITE_OOOHANDLER_H

#include "../handler.h"

namespace rtl {
  class OUString;
}

namespace Tellico {
  namespace Cite {

/**
 * @author Robby Stephenson
 */
class OOOHandler : public Handler {
public:
  OOOHandler();

  virtual State state() const;
  virtual bool connect();
  virtual bool cite(Cite::Map& fields);

private:
//  static QString OUString2Q(const rtl::OUString& str);
//  static rtl::OUString QString2OU(const QString& str);
  static Cite::Map s_fieldsMap;
  static void buildFieldsMap();

  Cite::Map convertFields(Cite::Map& values);

  class Interface;
  Interface* m_interface;
  // mutable since I want to change it inside state()
  mutable State m_state;
};

  } // end namespace
} // end namespace

#endif
