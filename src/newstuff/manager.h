/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
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

class KArchiveDirectory;
class KURL;
class KTempFile;

#include <qobject.h>
#include <qptrlist.h>

class QStringList;

namespace KNS {
  class Entry;
}
namespace KIO {
  class Job;
}

namespace Tellico {
  namespace NewStuff {

class NewScript;

enum DataType {
  EntryTemplate,
  DataScript
};

enum InstallStatus {
  NotInstalled,
  OldVersion,
  Current
};

struct DataSourceInfo {
  DataSourceInfo() : isUpdate(false) {}
  QString specFile; // full path of .spec file
  QString sourceName;
  QString sourceExec; // full executable path of script
  bool isUpdate : 1; // whether the info is for an updated source
};

class Manager : public QObject {
Q_OBJECT

public:
  Manager(QObject* parent);
  ~Manager();

  void install(DataType type, KNS::Entry* entry);
  QPtrList<DataSourceInfo> dataSourceInfo() const { return m_infoList; }

  bool installTemplate(const KURL& url, const QString& entryName = QString::null);
  bool removeTemplate(const QString& name);

  bool installScript(const KURL& url);
  bool removeScript(const QString& name);

  static InstallStatus installStatus(KNS::Entry* entry);
  static bool checkCommonFile();

signals:
  void signalInstalled(KNS::Entry* entry);

private slots:
  void slotDownloadJobResult(KIO::Job* job);
  void slotInstallFinished();

private:
  static QStringList archiveFiles(const KArchiveDirectory* dir,
                                  const QString& path = QString::null);

  static QString findXSL(const KArchiveDirectory* dir);
  static QString findEXE(const KArchiveDirectory* dir);

  typedef QPair<KNS::Entry*, DataType> EntryPair;
  QMap<KIO::Job*, EntryPair> m_jobMap;
  QMap<KURL, QString> m_urlNameMap;
  QMap<const NewScript*, KNS::Entry*> m_scriptEntryMap;
  QPtrList<DataSourceInfo> m_infoList;
  KTempFile* m_tempFile;
};

  }
}

#endif
