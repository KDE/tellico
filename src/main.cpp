/***************************************************************************
    Copyright (C) 2001-2018 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>

#include "mainwindow.h"
#include "core/logger.h"
#include "translators/translators.h" // needed for file type enum
#include "tellico_debug.h"

#include <KAboutData>
#include <KLocalizedString>
#include <KCrash>
#include <kiconthemes_version.h>
#include <KIconTheme>
#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#include <KStyleManager>
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStandardPaths>
#include <QDir>

int main(int argc, char* argv[]) {
  /**
   * Trigger initialisation of proper icon theme
   * see https://invent.kde.org/frameworks/kiconthemes/-/merge_requests/136
   */
#if KICONTHEMES_VERSION >= QT_VERSION_CHECK(6, 3, 0)
  KIconTheme::initTheme();
#endif

#ifndef USE_KHTML
  QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
  if(!qEnvironmentVariableIsSet("QT_SCALE_FACTOR_ROUNDING_POLICY")) {
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
  }
#endif
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  QApplication app(argc, argv);
#if HAVE_STYLE_MANAGER
  /**
   * Trigger initialisation of proper application style
   * see https://invent.kde.org/frameworks/kconfigwidgets/-/merge_requests/239
   *
   * But avoid hardcoding Breeze if qt6ct is set
   * see https://bugs.kde.org/show_bug.cgi?id=496074
   */
  if(qEnvironmentVariable("QT_QPA_PLATFORMTHEME") != QLatin1String("qt6ct")) {
    KStyleManager::initStyle();
  }
#else
  /**
   * For Windows and macOS: use Breeze if available
   * Of all tested styles that works the best for us
  */
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
  QApplication::setStyle(QStringLiteral("breeze"));
#endif
#endif
  KLocalizedString::setApplicationDomain("tellico");
  app.setApplicationVersion(QStringLiteral(TELLICO_VERSION));

  Q_INIT_RESOURCE(icons);

  KCrash::initialize();

  // component name = "tellico" is same as bugs.kde.org product name
  KAboutData aboutData(QStringLiteral("tellico"), QStringLiteral("Tellico"),
                       QStringLiteral(TELLICO_VERSION), i18n("Tellico - collection management software, free and simple"),
                       KAboutLicense::GPL_V2,
                       i18n("(c) 2001, Robby Stephenson"),
                       QString(),
                       QStringLiteral("https://tellico-project.org"));
  aboutData.addAuthor(QStringLiteral("Robby Stephenson"), QString(), QStringLiteral("robby@periapsis.org"));
  aboutData.addAuthor(QStringLiteral("Mathias Monnerville"), i18n("Data source scripts"));
  aboutData.addAuthor(QStringLiteral("Regis Boudin"), QString(), QStringLiteral("regis@boudin.name"));
  aboutData.addAuthor(QStringLiteral("Petri Damstén"), QString(), QStringLiteral("damu@iki.fi"));
  aboutData.addAuthor(QStringLiteral("Sebastian Held"), QString());

  aboutData.addCredit(QStringLiteral("Virginie Quesnay"), i18n("Icons"));
  aboutData.addCredit(QStringLiteral("Amarok"), i18n("Code examples and general inspiration"),
                      QString(), QStringLiteral("https://amarok.kde.org"));
  aboutData.addCredit(QStringLiteral("Greg Ward"), i18n("Author of btparse library"));
  aboutData.addCredit(QStringLiteral("Robert Gamble"), i18n("Author of libcsv library"));
  aboutData.addCredit(QStringLiteral("Valentin Lavrinenko"), i18n("Author of rtf2html library"));

  aboutData.addLicense(KAboutLicense::GPL_V3);
  aboutData.setOrganizationDomain("kde.org");
  aboutData.setDesktopFileName(QStringLiteral("org.kde.tellico"));

  QCommandLineParser parser;
  parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("nofile"), i18n("Do not reopen the last open file")));
  parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("bibtex"), i18n("Import <filename> as a bibtex file")));
  parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("mods"), i18n("Import <filename> as a MODS file")));
  parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("ris"), i18n("Import <filename> as a RIS file")));
  parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("pdf"), i18n("Import <filename> as a PDF file")));
  parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("log"), i18n("Log diagnostic output")));
  parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("logfile"), i18n("Write log output to <filename>"), QStringLiteral("logfile")));
  parser.addPositionalArgument(QStringLiteral("[filename]"), i18n("File to open"));

  aboutData.setupCommandLine(&parser);

  parser.process(app);
  aboutData.processCommandLine(&parser);
  KAboutData::setApplicationData(aboutData);

#ifndef NDEBUG
  QLoggingCategory::setFilterRules(QStringLiteral("tellico.debug = true"));
#endif
  // initialize logger
  Tellico::Logger::self();
  QString logFile = qEnvironmentVariable("TELLICO_LOGFILE");
  if(parser.isSet(QStringLiteral("logfile"))) {
    logFile = parser.value(QStringLiteral("logfile"));
  }
  if(logFile.isEmpty() && parser.isSet(QStringLiteral("log"))) {
    // use default log file location
    logFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/tellico_log.txt");
  }
  if(!logFile.isEmpty()) {
    Tellico::Logger::self()->setLogFile(logFile);
    myLog() << "Starting Tellico" << QStringLiteral(TELLICO_VERSION) << "at" << QDateTime::currentDateTime().toString(Qt::ISODate);
    myLog() << "Opening log file" << logFile;
  }

  if(app.isSessionRestored()) {
    myLog() << "Restoring previous session";
    kRestoreMainWindows<Tellico::MainWindow>();
  } else {
    Tellico::MainWindow* tellico = new Tellico::MainWindow();
    tellico->show();
    // slotInit gets called out of a QTimer signal
    // but it wasn't always completing in-time
    // so call it manually to ensure it has finished
    tellico->slotInit();

    QStringList args = parser.positionalArguments();
    if(args.count() > 0) {
      QLatin1String formatStr;
      Tellico::Import::Format format = Tellico::Import::TellicoXML;
      if(parser.isSet(QStringLiteral("bibtex"))) {
        format = Tellico::Import::Bibtex;
        formatStr = QLatin1String("bibtex");
      } else if(parser.isSet(QStringLiteral("mods"))) {
        format = Tellico::Import::MODS;
        formatStr = QLatin1String("mods");
      } else if(parser.isSet(QStringLiteral("ris"))) {
        format = Tellico::Import::RIS;
        formatStr = QLatin1String("ris");
      } else if(parser.isSet(QStringLiteral("pdf"))) {
        format = Tellico::Import::PDF;
        formatStr = QLatin1String("pdf");
      };
      const QUrl fileToLoad = QUrl::fromUserInput(args.at(0), QDir::currentPath());
      if(format == Tellico::Import::TellicoXML) {
        myLog() << "Opening" << fileToLoad.toDisplayString(QUrl::PreferLocalFile);
        tellico->slotFileOpen(fileToLoad);
      } else {
        myLog() << "Importing" << formatStr << "-" << fileToLoad.toDisplayString(QUrl::PreferLocalFile);
        tellico->importFile(format, fileToLoad, Tellico::Import::Replace);
        for(int i = 1; i < args.count(); ++i) {
          myLog() << "Appending" << fileToLoad.toDisplayString(QUrl::PreferLocalFile);
          tellico->importFile(format, fileToLoad, Tellico::Import::Append);
        }
      }
    } else {
      // bit of a hack, I just want the --nofile option
      // if --nofile is NOT passed, then the file option is set
      // is it's set, then go ahead and check for opening previous file
      tellico->initFileOpen(parser.isSet(QStringLiteral("nofile")));
    }
  }

  return app.exec();
}
