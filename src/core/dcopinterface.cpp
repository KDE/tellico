/***************************************************************************
    copyright            : (C) 2004-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "dcopinterface.h"
#include "../latin1literal.h"
#include "../controller.h"
#include "../tellico_kernel.h"
#include "../document.h"
#include "../collection.h"
#include "../translators/bibtexhandler.h"

using Tellico::ApplicationInterface;
using Tellico::CollectionInterface;

Tellico::Import::Action ApplicationInterface::actionType(const QString& actionName) {
  QString name = actionName.lower();
  if(name == Latin1Literal("append")) {
    return Import::Append;
  } else if(name == Latin1Literal("merge")) {
    return Import::Merge;
  }
  return Import::Replace;
}

QValueList<long> ApplicationInterface::selectedEntries() const {
  QValueList<long> ids;
  Data::EntryVec entries = Controller::self()->selectedEntries();
  for(Data::EntryVecIt entry = entries.begin(); entry != entries.end(); ++entry) {
    ids << entry->id();
  }
  return ids;
}

QValueList<long> ApplicationInterface::filteredEntries() const {
  QValueList<long> ids;
  Data::EntryVec entries = Controller::self()->visibleEntries();
  for(Data::EntryVecIt entry = entries.begin(); entry != entries.end(); ++entry) {
    ids << entry->id();
  }
  return ids;
}

long CollectionInterface::addEntry() {
  Data::CollPtr coll = Data::Document::self()->collection();
  if(!coll) {
    return -1;
  }
  Data::EntryPtr entry = new Data::Entry(coll);
  Kernel::self()->addEntries(entry, false);
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
  Kernel::self()->removeEntries(entry);
  return coll->entryById(id_) == 0;
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
  Data::EntryVec entries = Controller::self()->selectedEntries();
  for(Data::EntryVecIt entry = entries.begin(); entry != entries.end(); ++entry) {
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
  return BibtexHandler::bibtexKeys(entry).first();
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
  Data::EntryPtr oldEntry = new Data::Entry(*entry);
  if(!entry->setField(fieldName_, value_)) {
    return false;
  }
  Kernel::self()->modifyEntries(oldEntry, entry);
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
  Data::EntryPtr oldEntry = new Data::Entry(*entry);
  QStringList values = entry->fields(fieldName_, false);
  QStringList newValues = values;
  newValues << value_;
  if(!entry->setField(fieldName_, newValues.join(QString::fromLatin1("; ")))) {
    return false;
  }
  Kernel::self()->modifyEntries(oldEntry, entry);
  return true;
}
