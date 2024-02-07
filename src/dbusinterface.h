/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_DBUSINTERFACE_H
#define TELLICO_DBUSINTERFACE_H

#include "../translators/translators.h"

#include <QObject>
#include <QUrl>
#include <QStringList>

// the entry id is typedef'd to an int, but we need to use an int for DBUS

namespace Tellico {

class MainWindow;

class ApplicationInterface : public QObject {
Q_OBJECT
Q_CLASSINFO("D-Bus Interface", "org.kde.tellico")

public:
  ApplicationInterface(MainWindow* parent);

public Q_SLOTS:
  Q_SCRIPTABLE bool importTellico(const QString& file, const QString& action)
    { return importFile(Import::TellicoXML, file, actionType(action)); }
  Q_SCRIPTABLE bool importBibtex(const QString& file, const QString& action)
    { return importFile(Import::Bibtex, file, actionType(action)); }
  Q_SCRIPTABLE bool importMODS(const QString& file, const QString& action)
    { return importFile(Import::MODS, file, actionType(action)); }
  Q_SCRIPTABLE bool importRIS(const QString& file, const QString& action)
    { return importFile(Import::RIS, file, actionType(action)); }
  Q_SCRIPTABLE bool importPDF(const QString& file, const QString& action)
    { return importFile(Import::PDF, file, actionType(action)); }

  Q_SCRIPTABLE bool exportXML(const QString& file, bool filtered=false)
    { return exportCollection(Export::TellicoXML, file, filtered); }
  Q_SCRIPTABLE bool exportZip(const QString& file, bool filtered=false)
    { return exportCollection(Export::TellicoZip, file, filtered); }
  Q_SCRIPTABLE bool exportBibtex(const QString& file, bool filtered=false)
    { return exportCollection(Export::Bibtex, file, filtered); }
  Q_SCRIPTABLE bool exportHTML(const QString& file, bool filtered=false)
    { return exportCollection(Export::HTML, file, filtered); }
  Q_SCRIPTABLE bool exportCSV(const QString& file, bool filtered=false)
    { return exportCollection(Export::CSV, file, filtered); }

  Q_SCRIPTABLE QList<int> selectedEntries();
  Q_SCRIPTABLE QList<int> filteredEntries();

  Q_SCRIPTABLE virtual void openFile(const QString& file);
  Q_SCRIPTABLE virtual void setFilter(const QString& text);
  Q_SCRIPTABLE virtual bool showEntry(int id);

private:
  virtual bool importFile(Import::Format format, const QString& file, Import::Action action);
  virtual bool exportCollection(Export::Format format, const QString& file, bool filtered);

  Import::Action actionType(const QString& actionName);

  MainWindow* m_mainWindow;
};

class CollectionInterface : public QObject {
Q_OBJECT
Q_CLASSINFO("D-Bus Interface", "org.kde.tellico")

public:
  CollectionInterface(QObject* parent);

public Q_SLOTS:
  Q_SCRIPTABLE int addEntry();
  Q_SCRIPTABLE bool removeEntry(int entryID);

  Q_SCRIPTABLE QStringList allValues(const QString& fieldName);
  Q_SCRIPTABLE QStringList entryValues(int entryID, const QString& fieldName);
  Q_SCRIPTABLE QStringList selectedBibtexKeys();
  Q_SCRIPTABLE QString entryBibtexKey(int entryID);

  Q_SCRIPTABLE bool setEntryValue(int entryID, const QString& fieldName, const QString& value);
  Q_SCRIPTABLE bool addEntryValue(int entryID, const QString& fieldName, const QString& value);
};

} // end namespace
#endif
