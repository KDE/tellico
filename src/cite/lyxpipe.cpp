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

#include "lyxpipe.h"
#include "../collection.h"
#include "../translators/bibtexhandler.h"
#include "../tellico_kernel.h"
#include "../core/tellico_config.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kde_file.h>

#include <QFile>
#include <QTextStream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using Tellico::Cite::Lyxpipe;

Lyxpipe::Lyxpipe() : Action() {
}

bool Lyxpipe::cite(Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    return false;
  }

  Data::CollPtr coll = entries_.front()->collection();
  if(!coll || coll->type() != Data::Collection::Bibtex) {
    myDebug() << "Lyxpipe::cite() - collection must be a bibliography!" << endl;
    return false;
  }

  QString lyxpipe = Config::lyxpipe();
  lyxpipe += QLatin1String(".in");
//  myDebug() << "Lyxpipe::cite() - " << lyxpipe << endl;

  QString errorStr = i18n("<qt>Tellico is unable to write to the server pipe at <b>%1</b>.</qt>", lyxpipe);

  QFile file(lyxpipe);
  if(!file.exists()) {
    Kernel::self()->sorry(errorStr);
    return false;
  }

  int pipeFd = KDE_open(QFile::encodeName(lyxpipe), O_WRONLY);
  if(!file.open(pipeFd, QIODevice::WriteOnly)) {
    Kernel::self()->sorry(errorStr);
    ::close(pipeFd);
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
    myDebug() << "Lyxpipe::cite() - no available bibtex keys!" << endl;
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
  ::close(pipeFd);
  return true;
}
