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

#ifndef TELLICO_GUILINEEDIT_H
#define TELLICO_GUILINEEDIT_H

#include <KLineEdit>

#include <QString>

class KAction;
class KSpell;
namespace Sonnet {
  class Dialog;
}

class QFocusEvent;
class QPaintEvent;
class QContextMenuEvent;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class LineEdit : public KLineEdit {
Q_OBJECT

public:
  LineEdit(QWidget* parent);

  // by default, spell check is not allowed, and no popupmenu item is created
  void setAllowSpellCheck(bool b) { m_allowSpellCheck = b; }
  // spell check may be allowed but disabled
  void setEnableSpellCheck(bool b) { m_enableSpellCheck = b; }

protected:
  virtual void contextMenuEvent(QContextMenuEvent* event);

private slots:
  void slotCheckSpelling();
  void slotSpellCheckDone(const QString& text);
  void spellCheckerMisspelling(const QString& text, int pos);
  void spellCheckerCorrected(const QString& oldText, int pos, const QString& newText);

private:
  KAction* m_spellAction;
  bool m_allowSpellCheck;
  bool m_enableSpellCheck;
  Sonnet::Dialog* m_sonnetDialog;
};

  } // end namespace
} // end namespace
#endif
