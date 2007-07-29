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

#include "lineedit.h"

#include <kstdaction.h>
#include <kactioncollection.h>
#include <kspell.h>

#include <qapplication.h>
#include <qpainter.h>
#include <qpopupmenu.h>

using Tellico::GUI::LineEdit;

LineEdit::LineEdit(QWidget* parent_, const char* name_) : KLineEdit(parent_, name_)
    , m_drawHint(false)
    , m_allowSpellCheck(false)
    , m_enableSpellCheck(true)
    , m_spell(0) {
  m_spellAction = KStdAction::spelling(this, SLOT(slotCheckSpelling()), new KActionCollection(this));
}

void LineEdit::clear() {
  KLineEdit::clear();
  m_drawHint = true;
  repaint();
}

void LineEdit::setText(const QString& text_) {
  m_drawHint = text_.isEmpty();
  repaint();
  KLineEdit::setText(text_);
}

void LineEdit::setHint(const QString& hint_) {
  m_hint = hint_;
  m_drawHint = text().isEmpty();
  repaint();
}

void LineEdit::focusInEvent(QFocusEvent* event_) {
  if(m_drawHint) {
    m_drawHint = false;
    repaint();
  }
  KLineEdit::focusInEvent(event_);
}

void LineEdit::focusOutEvent(QFocusEvent* event_) {
  if(text().isEmpty()) {
    m_drawHint = true;
    repaint();
  }
  KLineEdit::focusOutEvent(event_);
}

void LineEdit::drawContents(QPainter* painter_) {
  // draw the regular line edit first
  KLineEdit::drawContents(painter_);

  // no need to draw anything else if in focus or no hint
  if(hasFocus() || !m_drawHint || m_hint.isEmpty() || !text().isEmpty()) {
    return;
  }

  // save current pen
  QPen oldPen = painter_->pen();

  // follow lead of kdepim and amarok, use disabled text color
  painter_->setPen(palette().color(QPalette::Disabled, QColorGroup::Text));

  QRect rect = contentsRect();
  // again, follow kdepim and amarok lead, and pad by 2 pixels
  rect.rLeft() += 2;
  painter_->drawText(rect, AlignAuto | AlignVCenter, m_hint);

  // reset pen
  painter_->setPen(oldPen);
}

QPopupMenu* LineEdit::createPopupMenu() {
  QPopupMenu* popup = KLineEdit::createPopupMenu();

  if(!popup) {
    return popup;
  }

  if(m_allowSpellCheck && echoMode() == QLineEdit::Normal && !isReadOnly()) {
    popup->insertSeparator();

    m_spellAction->plug(popup);
    m_spellAction->setEnabled(m_enableSpellCheck && !text().isEmpty());
  }

  return popup;
}

void LineEdit::slotCheckSpelling() {
  delete m_spell;
  // re-use the action string to get translations
  m_spell = new KSpell(this, m_spellAction->text(),
                       this, SLOT(slotSpellCheckReady(KSpell*)), 0, true, true);

  connect(m_spell, SIGNAL(death()),
          SLOT(spellCheckerFinished()));
  connect(m_spell, SIGNAL(misspelling( const QString &, const QStringList &, unsigned int)),
          SLOT(spellCheckerMisspelling( const QString &, const QStringList &, unsigned int)));
  connect(m_spell, SIGNAL(corrected(const QString &, const QString &, unsigned int)),
          SLOT(spellCheckerCorrected(const QString &, const QString &, unsigned int)));
}

void LineEdit::slotSpellCheckReady(KSpell* spell) {
  spell->check(text());
  connect(spell, SIGNAL(done(const QString&)), SLOT(slotSpellCheckDone(const QString&)));
}

void LineEdit::slotSpellCheckDone(const QString& newText) {
  if(newText != text()) {
    setText(newText);
  }
}

void LineEdit::spellCheckerFinished() {
  delete m_spell;
  m_spell = 0;
}

void LineEdit::spellCheckerMisspelling(const QString &text, const QStringList&, unsigned int pos) {
  setSelection(pos, pos + text.length());
}

void LineEdit::spellCheckerCorrected(const QString& oldWord, const QString& newWord, unsigned int pos) {
  if(oldWord != newWord) {
    setSelection(pos, pos + oldWord.length());
    insert(newWord);
  }
}

#include "lineedit.moc"
