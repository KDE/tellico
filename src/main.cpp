/***************************************************************************
                                   main.cpp
                             -------------------
    begin                : Wed Aug 29 21:00:54 CEST 2001
    copyright            : (C) 2001, 2002, 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bookcase.h"

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

static const char* description =
  I18N_NOOP("Bookcase - a book collection manager for KDE");

static const char* version = VERSION;

static KCmdLineOptions options[] = {
  { "+[filename]", I18N_NOOP("File to open"), 0 },
  { "nofile", I18N_NOOP("Do not reopen the last open file"), 0 },
  KCmdLineLastOption
};

int main(int argc, char* argv[]) {
  KAboutData aboutData("bookcase", I18N_NOOP("Bookcase"),
                       version, description, KAboutData::License_GPL,
                       "(c) 2001-2003, Robby Stephenson", 0,
                       "http://www.periapsis.org/bookcase/", "robby@periapsis.org");
  aboutData.addAuthor("Robby Stephenson", 0, "robby@periapsis.org");
  aboutData.addCredit("Greg Ward", I18N_NOOP("Author of btparse library"),
                      0, "http://www.gerg.ca");
  
  KCmdLineArgs::init(argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions(options);

  KApplication app;
 
  if(app.isRestored()) {
    RESTORE(Bookcase);
  } else {
    Bookcase* bookcase = new Bookcase();
    bookcase->show();
    bookcase->slotShowTipOfDay(false);

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    if(args->count() > 0) {
      bookcase->slotFileOpen(args->url(0));
    } else if(args->isSet("file")) {
      // bit of a hack, I just want the --nofile option
      // if --nofile is NOT passed, then the file option is set
      // is it's set, then go ahead and check for opening previous file
      bookcase->slotInitFileOpen();
    }
    args->clear();
  }

  return app.exec();
}  
