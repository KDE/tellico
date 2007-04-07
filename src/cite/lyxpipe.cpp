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

#include "lyxpipe.h"
#include "../collection.h"
#include "../translators/bibtexhandler.h"
#include "../tellico_kernel.h"
#include "../core/tellico_config.h"

#include <klocale.h>

#include <qfile.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using Tellico::Cite::Lyxpipe;

Lyxpipe::Lyxpipe() : Action() {
}

bool Lyxpipe::cite(Data::EntryVec entries_) {
  if(entries_.isEmpty()) {
    return false;
  }

  Data::CollPtr coll = entries_.front()->collection();
  if(!coll || coll->type() != Data::Collection::Bibtex) {
    myDebug() << "Lyxpipe::cite() - collection must be a bibliography!" << endl;
    return false;
  }

  QString lyxpipe = Config::lyxpipe();
  lyxpipe += QString::fromLatin1(".in");
//  myDebug() << "Lyxpipe::cite() - " << lyxpipe << endl;

  QString errorStr = i18n("<qt>Tellico is unable to write to the server pipe at <b>%1</b>.</qt>").arg(lyxpipe);

  QFile file(lyxpipe);
  if(!file.exists()) {
    Kernel::self()->sorry(errorStr);
    return false;
  }

  int pipeFd = ::open(QFile::encodeName(lyxpipe), O_WRONLY);
  if(!file.open(IO_WriteOnly, pipeFd)) {
    Kernel::self()->sorry(errorStr);
    ::close(pipeFd);
    return false;
  }

  QString output;
  QTextStream ts(&file);
  for(Data::EntryVecIt it = entries_.begin(); it != entries_.end(); ++it) {
    QString s = BibtexHandler::bibtexKey(it.data());
    if(s.isEmpty()) {
      continue;
    }
    output += s;
    if(!it.nextEnd() && !output.isEmpty()) {
      // pybliographer uses comma-space, and pyblink expects the space there
      output += QString::fromLatin1(", ");
    }
  }
  if(output.isEmpty()) {
    myDebug() << "Lyxpipe::cite() - no available bibtex keys!" << endl;
    return false;
  }

//  ts << "LYXSRV:tellico:hello\n";
  ts << "LYXCMD:tellico:citation-insert:";
  ts << output.local8Bit();
  ts << "\n";
//  ts << "LYXSRV:tellico:bye\n";
  file.flush();
  file.close();
  ::close(pipeFd);
  return true;
}
