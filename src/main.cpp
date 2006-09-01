/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "mainwindow.h"
#include "translators/translators.h" // needed for file type enum

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

namespace {
  static const char* description = I18N_NOOP("Tellico - a collection manager for KDE");
  static const char* version = VERSION;

  static KCmdLineOptions options[] = {
    { "nofile", I18N_NOOP("Do not reopen the last open file"), 0 },
    { "bibtex", I18N_NOOP("Import <filename> as a bibtex file"), 0 },
    { "mods", I18N_NOOP("Import <filename> as a MODS file"), 0 },
    { "ris", I18N_NOOP("Import <filename> as a RIS file"), 0 },
    { "+[filename]", I18N_NOOP("File to open"), 0 },
    KCmdLineLastOption
  };
}

int main(int argc, char* argv[]) {
  KAboutData aboutData("tellico", "Tellico",
                       version, description, KAboutData::License_GPL,
                       "(c) 2001-2006, Robby Stephenson", 0,
                       "http://www.periapsis.org/tellico/", "tellico-users@forge.novell.com");
  aboutData.addAuthor("Robby Stephenson", 0, "robby@periapsis.org");
  aboutData.addCredit("Mathias Monnerville", I18N_NOOP("Data source scripts"),
                      0, 0);
  aboutData.addCredit("Virginie Quesnay", I18N_NOOP("Icons"),
                      0, 0);
  aboutData.addCredit("Greg Ward", I18N_NOOP("Author of btparse library"),
                      0, "http://www.gerg.ca");
  aboutData.addCredit("Amarok", I18N_NOOP("Code examples and general inspiration"),
                      0, "http://amarok.kde.org");

  KCmdLineArgs::init(argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions(options);

  KApplication app;

  if(app.isRestored()) {
    RESTORE(Tellico::MainWindow);
  } else {
    Tellico::MainWindow* tellico = new Tellico::MainWindow();
    tellico->show();
    tellico->slotShowTipOfDay(false);

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    if(args->count() > 0) {
      if(args->isSet("bibtex")) {
        tellico->importFile(Tellico::Import::Bibtex, args->url(0), Tellico::Import::Replace);
      } else if(args->isSet("mods")) {
        tellico->importFile(Tellico::Import::MODS, args->url(0), Tellico::Import::Replace);
      } else if(args->isSet("ris")) {
        tellico->importFile(Tellico::Import::RIS, args->url(0), Tellico::Import::Replace);
      } else {
        tellico->slotFileOpen(args->url(0));
      }
    } else {
      // bit of a hack, I just want the --nofile option
      // if --nofile is NOT passed, then the file option is set
      // is it's set, then go ahead and check for opening previous file
      tellico->initFileOpen(!args->isSet("file"));
    }
    args->clear(); // some memory savings
  }

  return app.exec();
}
