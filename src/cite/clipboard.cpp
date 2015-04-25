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

#include "clipboard.h"
#include "../collection.h"
#include "../utils/bibtexhandler.h"

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

