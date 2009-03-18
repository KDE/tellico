/***************************************************************************
    copyright            : (C) 2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "cursorsaver.h"

#include <kapplication.h>

Tellico::GUI::CursorSaver::CursorSaver(const QCursor& cursor_) : m_restored(false) {
  kapp->setOverrideCursor(cursor_);
}

Tellico::GUI::CursorSaver::~CursorSaver() {
  if(!m_restored) {
    kapp->restoreOverrideCursor();
  }
}

void Tellico::GUI::CursorSaver::restore() {
  kapp->restoreOverrideCursor();
  m_restored = true;
}
