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

#ifndef TELLICO_FIELDCOMMAND_H
#define TELLICO_FIELDCOMMAND_H

#include "../datavectors.h"

#include <QUndoCommand>

namespace Tellico {
  namespace Command {

/**
 * @author Robby Stephenson
 */
class FieldCommand : public QUndoCommand  {

public:
  enum Mode {
    FieldAdd,
    FieldModify,
    FieldRemove
  };

  FieldCommand(Mode mode, Data::CollPtr coll, Data::FieldPtr activeField, Data::FieldPtr oldField=Data::FieldPtr());
  FieldCommand(QUndoCommand* parent, Mode mode, Data::CollPtr coll, Data::FieldPtr activeField, Data::FieldPtr oldField=Data::FieldPtr());

  virtual void redo() override;
  virtual void undo() override;

private:
  void init();

  Mode m_mode;
  Data::CollPtr m_coll;
  Data::FieldPtr m_activeField;
  Data::FieldPtr m_oldField;
};

  } // end namespace
}

#endif
