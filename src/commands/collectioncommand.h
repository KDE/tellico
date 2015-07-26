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

#ifndef TELLICO_COLLECTIONCOMMAND_H
#define TELLICO_COLLECTIONCOMMAND_H

#include "../datavectors.h"

#include <QUndoCommand>
#include <QUrl>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class CollectionCommand : public QUndoCommand {
public:
  enum Mode {
    Append,
    Merge,
    Replace
  };

  CollectionCommand(Mode mode, Data::CollPtr currentColl, Data::CollPtr newColl);
  ~CollectionCommand();

  virtual void redo();
  virtual void undo();

private:
  void copyFields();

  Mode m_mode;
  Data::CollPtr m_origColl;
  Data::CollPtr m_newColl;

  QUrl m_origURL;
  Data::FieldList m_origFields;
  Data::MergePair m_mergePair;
  // for the Replace case, the collection that got replaced needs to be cleared
  enum CleanupMode {
    DoNothing, ClearOriginal, ClearNew
  };
  CleanupMode m_cleanup;
};

  } // end namespace
}

#endif
