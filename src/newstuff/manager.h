/***************************************************************************
    copyright            : (C) 2006-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_NEWSTUFF_MANAGER_H
#define TELLICO_NEWSTUFF_MANAGER_H

#include <QObject>
#include <QMap>

class KArchiveDirectory;
class KUrl;
class KTemporaryFile;
class QStringList;

namespace Tellico {
  namespace NewStuff {

class Manager : public QObject {
Q_OBJECT

public:
  static Manager* self();
  QMap<QString, QString> userTemplates();
  bool installTemplate(const QString& file);
  bool removeTemplateByName(const QString& name);
  bool removeTemplate(const QString& file, bool manual = false);

  bool installScript(const QString& file);
  bool removeScriptByName(const QString& name);
  bool removeScript(const QString& file, bool manual = false);

private:
  friend class ManagerSingleton;

  explicit Manager(QObject* parent = 0);
  ~Manager();

  static QStringList archiveFiles(const KArchiveDirectory* dir,
                                  const QString& path = QString());
  static bool checkCommonFile();
  static void removeNewStuffFile(const QString& file);
  static QString findXSL(const KArchiveDirectory* dir);
  static QString findEXE(const KArchiveDirectory* dir);
};

  }
}

#endif
