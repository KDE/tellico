/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#include "mainwindow.h"
#include "translators/translators.h" // needed for file type enum

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

namespace {
  static const char* version = "2.0pre2";
}

int main(int argc, char* argv[]) {
  KAboutData aboutData("tellico", 0, ki18n("Tellico"),
                       version, ki18n("Tellico - a collection manager for KDE"), KAboutData::License_GPL_V2,
                       ki18n("(c) 2001-2009, Robby Stephenson"), KLocalizedString(),
                       "http://tellico-project.org", "tellico-users@kde.org");
  aboutData.addAuthor(ki18n("Robby Stephenson"), KLocalizedString(), "robby@periapsis.org");
  aboutData.addAuthor(ki18n("Mathias Monnerville"), ki18n("Data source scripts"));
  aboutData.addAuthor(ki18n("Regis Boudin"), KLocalizedString());
  aboutData.addAuthor(ki18n("Petri Damstén"), KLocalizedString(), "damu@iki.fi");

  aboutData.addCredit(ki18n("Virginie Quesnay"), ki18n("Icons"));
  aboutData.addCredit(ki18n("Amarok"), ki18n("Code examples and general inspiration"),
                      QByteArray(), "http://amarok.kde.org");
  aboutData.addCredit(ki18n("Greg Ward"), ki18n("Author of btparse library"));
  aboutData.addCredit(ki18n("Robert Gamble"), ki18n("Author of libcsv library"));
  aboutData.addCredit(ki18n("Valentin Lavrinenko"), ki18n("Author of rtf2html library"));

  aboutData.addLicense(KAboutData::License_GPL_V3);

  KCmdLineOptions options;
  options.add("nofile", ki18n("Do not reopen the last open file"));
  options.add("bibtex", ki18n("Import <filename> as a bibtex file"));
  options.add("mods", ki18n("Import <filename> as a MODS file"));
  options.add("ris", ki18n("Import <filename> as a RIS file"));
  options.add("+[filename]", ki18n("File to open"));

  KCmdLineArgs::init(argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions(options);

  KApplication app;

  if(app.isSessionRestored()) {
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
