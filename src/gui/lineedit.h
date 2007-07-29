/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
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

#include <klineedit.h>

#include <qstring.h>

class KAction;
class KSpell;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class LineEdit : public KLineEdit {
Q_OBJECT

public:
  LineEdit(QWidget* parent = 0, const char* name = 0);

  virtual void setText(const QString& text);
  void setHint(const QString& hint);

  // by default, spell check is not allowed, and no popupmenu item is created
  void setAllowSpellCheck(bool b) { m_allowSpellCheck = b; }
  // spell check may be allowed but disabled
  void setEnableSpellCheck(bool b) { m_enableSpellCheck = b; }

public slots:
  void clear();

protected:
  virtual void focusInEvent(QFocusEvent* event);
  virtual void focusOutEvent(QFocusEvent* event);
  virtual void drawContents(QPainter* painter);
  virtual QPopupMenu* createPopupMenu();

private slots:
  void slotCheckSpelling();
  void slotSpellCheckReady(KSpell* spell);
  void slotSpellCheckDone(const QString& text);
  void spellCheckerMisspelling(const QString& text, const QStringList&, unsigned int pos);
  void spellCheckerCorrected(const QString& oldText, const QString& newText, unsigned int pos);
  void spellCheckerFinished();

private:
  QString m_hint;
  bool m_drawHint;
  KAction* m_spellAction;
  bool m_allowSpellCheck;
  bool m_enableSpellCheck;
  KSpell* m_spell;
};

  } // end namespace
} // end namespace
#endif
