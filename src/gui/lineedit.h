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

#ifndef TELLICO_GUILINEEDIT_H
#define TELLICO_GUILINEEDIT_H

#include <KLineEdit>

#include <QString>

class QAction;
class KSpell;
namespace Sonnet {
  class Dialog;
}

class QContextMenuEvent;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class LineEdit : public KLineEdit { //krazy:exclude=qclasses
Q_OBJECT

public:
  LineEdit(QWidget* parent);

  // by default, spell check is not allowed, and no popupmenu item is created
  void setAllowSpellCheck(bool b) { m_allowSpellCheck = b; }
  // spell check may be allowed but disabled
  void setEnableSpellCheck(bool b) { m_enableSpellCheck = b; }

protected:
  virtual void contextMenuEvent(QContextMenuEvent* event) override;

private Q_SLOTS:
  void slotCheckSpelling();
  void slotSpellCheckDone(const QString& text);
  void spellCheckerMisspelling(const QString& text, int pos);
  void spellCheckerCorrected(const QString& oldText, int pos, const QString& newText);

private:
  QAction* m_spellAction;
  bool m_allowSpellCheck;
  bool m_enableSpellCheck;
  Sonnet::Dialog* m_sonnetDialog;
};

  } // end namespace
} // end namespace
#endif
