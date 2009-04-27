/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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
