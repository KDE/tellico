/***************************************************************************
    copyright            : (C) 2004-2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_DBUSINTERFACE_H
#define TELLICO_DBUSINTERFACE_H

#include "../translators/translators.h"

#include <kurl.h>

#include <QStringList>

// DBus doesn't have a type for int int, which is the type of the entry id right now
// go ahead and downcast to an int

namespace Tellico {

class MainWindow;

class ApplicationInterface : public QObject {
Q_OBJECT
Q_CLASSINFO("D-Bus Interface", "org.kde.tellico")

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

  Q_SCRIPTABLE QList<int> selectedEntries() const;
  Q_SCRIPTABLE QList<int> filteredEntries() const;

  Q_SCRIPTABLE virtual void openFile(const QString& file);
  Q_SCRIPTABLE virtual void setFilter(const QString& text);
  Q_SCRIPTABLE virtual bool showEntry(int id);

private:
  virtual bool importFile(Import::Format format, const KUrl& url, Import::Action action);
  virtual bool exportCollection(Export::Format format, const KUrl& url);

  Import::Action actionType(const QString& actionName);

  MainWindow* m_mainWindow;
};

class CollectionInterface : public QObject {
Q_OBJECT
Q_CLASSINFO("D-Bus Interface", "org.kde.tellico")

public:
  CollectionInterface(QObject* parent);

public slots:
  Q_SCRIPTABLE int addEntry();
  Q_SCRIPTABLE bool removeEntry(int entryID);

  Q_SCRIPTABLE QStringList allValues(const QString& fieldName) const;
  Q_SCRIPTABLE QStringList entryValues(int entryID, const QString& fieldName) const;
  Q_SCRIPTABLE QStringList selectedBibtexKeys() const;
  Q_SCRIPTABLE QString entryBibtexKey(int entryID) const;

  Q_SCRIPTABLE bool setEntryValue(int entryID, const QString& fieldName, const QString& value);
  Q_SCRIPTABLE bool addEntryValue(int entryID, const QString& fieldName, const QString& value);
};

} // end namespace
#endif
