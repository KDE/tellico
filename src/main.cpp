/***************************************************************************
                                   main.cpp
                             -------------------
    begin                : Wed Aug 29 21:00:54 CEST 2001
    copyright            : (C) 2001 by Robby Stephenson
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

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

//#include <qtimer.h>

static const char* description =
  I18N_NOOP("Bookcase - a book collection manager for KDE");

static const char* version = VERSION;

static KCmdLineOptions options[] = {
  { "+[filename]", I18N_NOOP("File to open"), 0 },
  { 0, 0, 0 }
};

int main(int argc, char* argv[]) {
  KAboutData aboutData("bookcase", I18N_NOOP("Bookcase"),
    version, description, KAboutData::License_GPL,
    "(c) 2001-2003, Robby Stephenson", 0,
    "http://www.periapsis.org/bookcase/", "robby@periapsis.org");
  aboutData.addAuthor("Robby Stephenson", 0, "robby@periapsis.org");
  
  KCmdLineArgs::init(argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions(options);

  KApplication app;
 
  if(app.isRestored()) {
    RESTORE(Bookcase);
  } else {
    Bookcase* bookcase = new Bookcase();
    bookcase->show();

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    if(args->count() > 0) {
//      QTimer::singleShot(0, bookcase, SLOT(slotFileOpen(args->url(0))));
      bookcase->slotFileOpen(args->url(0));
    } else {
//      QTimer::singleShot(0, bookcase, SLOT(slotInitFileOpen()));
      bookcase->slotInitFileOpen();
    }
    args->clear();
  }

  return app.exec();
}  
