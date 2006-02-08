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

#include "clipboard.h"
#include "../collection.h"
#include "../translators/bibtexhandler.h"

#include <klocale.h>

#include <qapplication.h>
#include <qclipboard.h>

using Tellico::Cite::Clipboard;

Clipboard::Clipboard() : Action() {
}

bool Clipboard::cite(Data::EntryVec entries_) {
  if(entries_.isEmpty()) {
    return false;
  }

  Data::CollPtr coll = entries_.front()->collection();
  if(!coll || coll->type() != Data::Collection::Bibtex) {
    return false;
  }

  QString s = QString::fromLatin1("\\cite{");
  for(Data::EntryVecIt it = entries_.begin(); it != entries_.end(); ++it) {
    s += BibtexHandler::bibtexKey(it.data());
    if(!it.nextEnd()) {
      s += QString::fromLatin1(", ");
    }
  }
  s += '}';

  QClipboard *cb = QApplication::clipboard();
  cb->setText(s, QClipboard::Clipboard);
  return true;
}

