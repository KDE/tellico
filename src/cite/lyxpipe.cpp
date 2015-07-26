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

#include "lyxpipe.h"
#include "../collection.h"
#include "../utils/bibtexhandler.h"
#include "../core/tellico_config.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QFile>
#include <QTextStream>

using Tellico::Cite::Lyxpipe;

Lyxpipe::Lyxpipe() : Action(), m_hasError(false) {
}

bool Lyxpipe::cite(Tellico::Data::EntryList entries_) {
  m_hasError = false;
  if(entries_.isEmpty()) {
    return false;
  }

  Data::CollPtr coll = entries_.front()->collection();
  if(!coll || coll->type() != Data::Collection::Bibtex) {
    myDebug() << "collection must be a bibliography!";
    return false;
  }

  QString lyxpipe = Config::lyxpipe();
  lyxpipe += QLatin1String(".in");
//  myDebug() << lyxpipe;

  m_errorString = i18n("<qt>Tellico is unable to write to the server pipe at <b>%1</b>.</qt>", lyxpipe);

  QFile file(lyxpipe);
  if(!file.exists()) {
    m_hasError = true;
    return false;
  }

  if(!file.open(QIODevice::WriteOnly)) {
    m_hasError = true;
    return false;
  }

  QString output;
  QTextStream ts(&file);
  foreach(Data::EntryPtr entry, entries_) {
    QString s = BibtexHandler::bibtexKey(entry);
    if(s.isEmpty()) {
      continue;
    }
    output += s;
    if(!output.isEmpty()) {
      // pybliographer uses comma-space, and pyblink expects the space there
      output += QLatin1String(", ");
    }
  }
  if(output.isEmpty()) {
    myDebug() << "no available bibtex keys!";
    return false;
  } else {
    output.truncate(output.length()-2); // remove last comma and space
  }

//  ts << "LYXSRV:tellico:hello\n";
  ts << "LYXCMD:tellico:citation-insert:";
  ts << output.toLocal8Bit();
  ts << "\n";
//  ts << "LYXSRV:tellico:bye\n";
  ts.flush();
  file.close();
  return true;
}

bool Lyxpipe::hasError() const {
  return m_hasError;
}

QString Lyxpipe::errorString() const {
  return m_hasError ? m_errorString : QString();
}
