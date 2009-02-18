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

#include "dbusinterface.h"
#include "../controller.h"
#include "../tellico_kernel.h"
#include "../document.h"
#include "../collection.h"
#include "../translators/bibtexhandler.h"
#include "../mainwindow.h"

#include <QDBusConnection>

using Tellico::ApplicationInterface;
using Tellico::CollectionInterface;

ApplicationInterface::ApplicationInterface(Tellico::MainWindow* parent_) : QObject(parent_), m_mainWindow(parent_) {
  QDBusConnection::sessionBus().registerObject("/tellico", this, QDBusConnection::ExportScriptableSlots);
}

Tellico::Import::Action ApplicationInterface::actionType(const QString& actionName) {
  QString name = actionName.toLower();
  if(name == QLatin1String("append")) {
    return Import::Append;
  } else if(name == QLatin1String("merge")) {
    return Import::Merge;
  }
  return Import::Replace;
}

QList<long> ApplicationInterface::selectedEntries() const {
  QList<long> ids;
  foreach(Data::EntryPtr entry, Controller::self()->selectedEntries()) {
    ids << entry->id();
  }
  return ids;
}

QList<long> ApplicationInterface::filteredEntries() const {
  QList<long> ids;
  foreach(Data::EntryPtr entry, Controller::self()->visibleEntries()) {
    ids << entry->id();
  }
  return ids;
}

void ApplicationInterface::openFile(const QString& file) {
  return m_mainWindow->openFile(file);
}

void ApplicationInterface::setFilter(const QString& text) {
  return m_mainWindow->setFilter(text);
}

bool ApplicationInterface::showEntry(long id)  {
  return m_mainWindow->showEntry(id);
}

bool ApplicationInterface::importFile(Tellico::Import::Format format, const KUrl& url, Tellico::Import::Action action) {
  return m_mainWindow->importFile(format, url, action);
}

bool ApplicationInterface::exportCollection(Tellico::Export::Format format, const KUrl& url) {
  return m_mainWindow->exportCollection(format, url);
}

CollectionInterface::CollectionInterface(QObject* parent_) : QObject(parent_) {
  QDBusConnection::sessionBus().registerObject("/collection", this, QDBusConnection::ExportScriptableSlots);
}

long CollectionInterface::addEntry() {
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll) {
    return -1;
  }
  Data::EntryPtr entry(new Data::Entry(coll));
  Kernel::self()->addEntries(Data::EntryList() << entry, false);
  return entry->id();
}

bool CollectionInterface::removeEntry(long id_) {
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll) {
    return false;
  }
  Data::EntryPtr entry = coll->entryById(id_);
  if(!entry) {
    return false;
  }
  Kernel::self()->removeEntries(Data::EntryList() << entry);
  return !coll->entryById(id_);
}

QStringList CollectionInterface::values(const QString& fieldName_) const {
  QStringList results;
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll) {
    return results;
  }
  Data::FieldPtr field = coll->fieldByName(fieldName_);
  if(!field) {
    field = coll->fieldByTitle(fieldName_);
  }
  if(!field) {
    return results;
  }
  Data::EntryList entries = Controller::self()->selectedEntries();
  foreach(Data::EntryPtr entry, entries) {
    results += entry->fields(field, false);
  }
  return results;
}

QStringList CollectionInterface::values(long id_, const QString& fieldName_) const {
  QStringList results;
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll) {
    return results;
  }
  Data::FieldPtr field = coll->fieldByName(fieldName_);
  if(!field) {
    field = coll->fieldByTitle(fieldName_);
  }
  if(!field) {
    return results;
  }
  Data::EntryPtr entry = coll->entryById(id_);
  if(entry) {
    results += entry->fields(field, false);
  }
  return results;
}

QStringList CollectionInterface::bibtexKeys() const {
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll || coll->type() != Data::Collection::Bibtex) {
    return QStringList();
  }
  return BibtexHandler::bibtexKeys(Controller::self()->selectedEntries());
}

QString CollectionInterface::bibtexKey(long id_) const {
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll || coll->type() != Data::Collection::Bibtex) {
    return QString();
  }
  Data::EntryPtr entry = coll->entryById(id_);
  if(!entry) {
    return QString();
  }
  return BibtexHandler::bibtexKeys(Data::EntryList() << entry).first();
}

bool CollectionInterface::setFieldValue(long id_, const QString& fieldName_, const QString& value_) {
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll) {
    return false;
  }
  Data::EntryPtr entry = coll->entryById(id_);
  if(!entry) {
    return false;
  }
  Data::EntryPtr oldEntry(new Data::Entry(*entry));
  if(!entry->setField(fieldName_, value_)) {
    return false;
  }
  Kernel::self()->modifyEntries(Data::EntryList() << oldEntry, Data::EntryList() << entry);
  return true;
}

bool CollectionInterface::addFieldValue(long id_, const QString& fieldName_, const QString& value_) {
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll) {
    return false;
  }
  Data::EntryPtr entry = coll->entryById(id_);
  if(!entry) {
    return false;
  }
  Data::EntryPtr oldEntry(new Data::Entry(*entry));
  QStringList values = entry->fields(fieldName_, false);
  QStringList newValues = values;
  newValues << value_;
  if(!entry->setField(fieldName_, newValues.join(QLatin1String("; ")))) {
    return false;
  }
  Kernel::self()->modifyEntries(Data::EntryList() << oldEntry, Data::EntryList() << entry);
  return true;
}
