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

#include "clipboard.h"
#include "../collection.h"
#include "../translators/bibtexhandler.h"

#include <klocale.h>

#include <QApplication>
#include <QClipboard>

using Tellico::Cite::Clipboard;

Clipboard::Clipboard() : Action() {
}

bool Clipboard::cite(Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    return false;
  }

  Data::CollPtr coll = entries_.front()->collection();
  if(!coll || coll->type() != Tellico::Data::Collection::Bibtex) {
    return false;
  }

  QString s = QLatin1String("\\cite{");
  foreach(Data::EntryPtr entry, entries_) {
      s += BibtexHandler::bibtexKey(entry);
    s += QLatin1String(", ");
  }
  s.truncate(s.length()-2); // remove last comma
  s += QLatin1Char('}');

  QClipboard* cb = QApplication::clipboard();
  cb->setText(s, QClipboard::Clipboard);
  return true;
}

