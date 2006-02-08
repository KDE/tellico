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

#include <kapplication.h>
#include <kconfig.h>

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
    return false;
  }

  KConfigGroupSaver saver(kapp->config(), QString::fromLatin1("Options - %1").arg(QString::fromLatin1("bibtex")));
  QString lyxpipe = kapp->config()->readPathEntry("lyxpipe", QString::fromLatin1("$HOME/.lyx/lyxpipe"));
  lyxpipe += QString::fromLatin1(".in");
//  kdDebug() << "MainWindow::slotCiteEntry() - " << lyxpipe << endl;

  QString errorStr = i18n("<qt>Tellico is unable to write to the server pipe at <b>%1</b>.</qt>").arg(lyxpipe);

  QCString pipe = QFile::encodeName(lyxpipe);
  if(!QFile::exists(pipe)) {
    Kernel::self()->sorry(errorStr);
    return false;
  }

  int pipeFd = ::open(pipe, O_WRONLY);
  QFile file(QString::fromUtf8(pipe));
  if(!file.open(IO_WriteOnly, pipeFd)) {
    Kernel::self()->sorry(errorStr);
    ::close(pipeFd);
    return false;
  }

  QTextStream ts(&file);
//  ts << "LYXSRV:tellico:hello\n";
  ts << "LYXCMD:tellico:citation-insert:";
  for(Data::EntryVecIt it = entries_.begin(); it != entries_.end(); ++it) {
    ts << BibtexHandler::bibtexKey(it.data()).local8Bit();
    if(!it.nextEnd()) {
      // pybliographer uses comma-space, and pyblink expects the space there
      ts << ", ";
    }
  }
  ts << "\n";
//  ts << "LYXSRV:tellico:bye\n";
  file.flush();
  file.close();
  ::close(pipeFd);
  return true;
}
