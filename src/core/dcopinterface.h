/***************************************************************************
    copyright            : (C) 2004-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_DCOPINTERFACE_H
#define TELLICO_DCOPINTERFACE_H

#include "../translators/translators.h"

#include <kurl.h>

#include <QStringList>

namespace Tellico {

class MainWindow;

class ApplicationInterface : public QObject {
Q_OBJECT
Q_CLASSINFO("D-Bus Interface", "org.tellico-project.application")

public:
  ApplicationInterface(MainWindow* parent);

public slots:
  Q_SCRIPTABLE bool importTellico(const QString& file, const QString& action)
    { return importFile(Import::TellicoXML, KUrl::fromPath(file), actionType(action)); }
  Q_SCRIPTABLE bool importBibtex(const QString& file, const QString& action)
    { return importFile(Import::Bibtex, KUrl::fromPath(file), actionType(action)); }
  Q_SCRIPTABLE bool importMODS(const QString& file, const QString& action)
    { return importFile(Import::MODS, KUrl::fromPath(file), actionType(action)); }
  Q_SCRIPTABLE bool importRIS(const QString& file, const QString& action)
    { return importFile(Import::RIS, KUrl::fromPath(file), actionType(action)); }

  Q_SCRIPTABLE bool exportXML(const QString& file)
    { return exportCollection(Export::TellicoXML, KUrl::fromPath(file)); }
  Q_SCRIPTABLE bool exportZip(const QString& file)
    { return exportCollection(Export::TellicoZip, KUrl::fromPath(file)); }
  Q_SCRIPTABLE bool exportBibtex(const QString& file)
    { return exportCollection(Export::Bibtex, KUrl::fromPath(file)); }
  Q_SCRIPTABLE bool exportHTML(const QString& file)
    { return exportCollection(Export::HTML, KUrl::fromPath(file)); }
  Q_SCRIPTABLE bool exportCSV(const QString& file)
    { return exportCollection(Export::CSV, KUrl::fromPath(file)); }
  Q_SCRIPTABLE bool exportPilotDB(const QString& file)
    { return exportCollection(Export::PilotDB, KUrl::fromPath(file)); }

  Q_SCRIPTABLE QList<long> selectedEntries() const;
  Q_SCRIPTABLE QList<long> filteredEntries() const;

  Q_SCRIPTABLE virtual void openFile(const QString& file);
  Q_SCRIPTABLE virtual void setFilter(const QString& text);
  Q_SCRIPTABLE virtual bool showEntry(long id);

private:
  virtual bool importFile(Import::Format format, const KUrl& url, Import::Action action);
  virtual bool exportCollection(Export::Format format, const KUrl& url);

  Import::Action actionType(const QString& actionName);

  MainWindow* m_mainWindow;
};

class CollectionInterface : public QObject {
Q_OBJECT
Q_CLASSINFO("D-Bus Interface", "org.tellico-project.collection")

public:
  CollectionInterface(QObject* parent);

public slots:
  Q_SCRIPTABLE long addEntry();
  Q_SCRIPTABLE bool removeEntry(long entryID);

  Q_SCRIPTABLE QStringList values(const QString& fieldName) const;
  Q_SCRIPTABLE QStringList values(long entryID, const QString& fieldName) const;
  Q_SCRIPTABLE QStringList bibtexKeys() const;
  Q_SCRIPTABLE QString bibtexKey(long entryID) const;

  Q_SCRIPTABLE bool setFieldValue(long entryID, const QString& fieldName, const QString& value);
  Q_SCRIPTABLE bool addFieldValue(long entryID, const QString& fieldName, const QString& value);
};

} // end namespace
#endif
