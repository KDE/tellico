/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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
