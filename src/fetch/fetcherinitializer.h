/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_FETCHERINITIALIZER_H
#define TELLICO_FETCH_FETCHERINITIALIZER_H

#include "fetchmanager.h"

namespace Tellico {
  namespace Fetch {

class ConfigWidget;

/**
 * @author Robby Stephenson
 */
class FetcherInitializer {

public:
  FetcherInitializer();
};

/**
 * Helper template for registering fetcher classes
 */
template <class Derived>
class RegisterFetcher {
public:
  static Tellico::Fetch::Fetcher::Ptr createInstance(QObject* parent) {
    return Tellico::Fetch::Fetcher::Ptr(new Derived(parent));
  }
  static QString getName() {
    return Derived::defaultName();
  }
  static QString getIcon() {
    return Derived::defaultIcon();
  }
  static StringHash getOptionalFields() {
    return Derived::allOptionalFields();
  }
  static Tellico::Fetch::ConfigWidget* createConfigWidget(QWidget* parent) {
    return new typename Derived::ConfigWidget(parent);
  }
  RegisterFetcher(int type) {
    Manager::FetcherFunction f;
    f.create = createInstance;
    f.name = getName;
    f.icon = getIcon;
    f.optionalFields = getOptionalFields;
    f.configWidget = createConfigWidget;
    Manager::self()->registerFunction(type, f);
  }
};

  }
}

#endif
