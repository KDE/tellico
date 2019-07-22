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

#ifndef TELLICO_CITE_ACTIONMANAGER_H
#define TELLICO_CITE_ACTIONMANAGER_H

#include "../datavectors.h"

namespace Tellico {
  namespace Cite {

enum CiteAction {
  CiteClipboard,
  CiteLyxpipe
};

/**
 * @author Robby Stephenson
 */
class Action {
public:
  Action() {}
  virtual ~Action() {}

  virtual CiteAction type() const = 0;
  virtual bool connect() { return true; }
  virtual bool cite(Data::EntryList entries) = 0;
  virtual bool hasError() const { return false; }
  virtual QString errorString() const { return QString(); }

private:
  Q_DISABLE_COPY(Action)
};

/**
 * @author Robby Stephenson
 */
class ActionManager {
public:
  static ActionManager* self();
  ~ActionManager();

  bool cite(CiteAction action, Data::EntryList entries);
  bool hasError() const;
  QString errorString() const;

private:
  ActionManager();
  bool connect(CiteAction action);

  Action* m_action;
};

  }
}

#endif
