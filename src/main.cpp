/* *************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Wed Aug 29 21:00:54 CEST 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#include "bookcase.h"

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

static const char *description =
  I18N_NOOP("Bookcase - a personal book collection manager for KDE");

static const char *version = VERSION;

static KCmdLineOptions options[] = {
  { "+[filename]", I18N_NOOP("file to open"), 0 },
  { 0, 0, 0 }
};

int main(int argc, char *argv[]) {
  KAboutData aboutData("bookcase", I18N_NOOP("Bookcase"),
    version, description, KAboutData::License_GPL,
    "(c) 2001, Robby Stephenson", 0,
    "http://radiojodi.com/bookcase/", "robby@radiojodi.com");
  aboutData.addAuthor("Robby Stephenson", 0, "robby@radiojodi.com");
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
      bookcase->slotFileOpen(args->url(0));
    }
    args->clear();
  }

  return app.exec();
}  
