/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#include "lineedit.h"

#include <KStandardAction>
#include <KActionCollection>
#include <Sonnet/Dialog>
#include <Sonnet/BackgroundChecker>

#include <QMenu>
#include <QContextMenuEvent>

using Tellico::GUI::LineEdit;

LineEdit::LineEdit(QWidget* parent_) : KLineEdit(parent_) //krazy:exclude=qclasses
    , m_allowSpellCheck(false)
    , m_enableSpellCheck(true)
    , m_sonnetDialog(nullptr) {
  m_spellAction = KStandardAction::spelling(this, &LineEdit::slotCheckSpelling, new KActionCollection(this));
}

void LineEdit::contextMenuEvent(QContextMenuEvent* event_) {
  QMenu* menu = createStandardContextMenu();

  if(!menu) {
    return;
  }

  if(m_allowSpellCheck && echoMode() == Normal && !isReadOnly()) {
    menu->addSeparator();
    menu->addAction(m_spellAction);
    m_spellAction->setEnabled(m_enableSpellCheck && !text().isEmpty());
  }

  menu->exec(event_->globalPos());
  delete menu;
}

void LineEdit::slotCheckSpelling() {
  delete m_sonnetDialog;
  m_sonnetDialog = new Sonnet::Dialog(new Sonnet::BackgroundChecker(this), this);


  void (Sonnet::Dialog::* doneString)(const QString&) = &Sonnet::Dialog::spellCheckDone;
  connect(m_sonnetDialog, doneString,
          this, &LineEdit::slotSpellCheckDone);
  connect(m_sonnetDialog, &Sonnet::Dialog::misspelling,
          this, &LineEdit::spellCheckerMisspelling);
  connect(m_sonnetDialog, &Sonnet::Dialog::replace,
          this, &LineEdit::spellCheckerCorrected);

  if(hasSelectedText()) {
    m_sonnetDialog->setBuffer(selectedText());
  } else {
    m_sonnetDialog->setBuffer(text());
  }

  m_sonnetDialog->show();
}

void LineEdit::slotSpellCheckDone(const QString& newText) {
  if(newText != text()) {
    setText(newText);
  }
  m_sonnetDialog->hide();
  m_sonnetDialog->deleteLater();
  m_sonnetDialog = nullptr;
}

void LineEdit::spellCheckerMisspelling(const QString &text, int pos) {
  setSelection(pos, pos + text.length());
}

void LineEdit::spellCheckerCorrected(const QString& oldWord, int pos, const QString& newWord) {
  if(oldWord != newWord) {
    setSelection(pos, pos + oldWord.length());
    insert(newWord);
  }
}
