/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "document.h"
#include "mainwindow.h" // needed for calling fileSave()
#include "collectionfactory.h"
#include "translators/tellicoimporter.h"
#include "translators/tellicozipexporter.h"
#include "entry.h"
#include "field.h"
#include "filehandler.h"
#include "controller.h"
#include "filter.h"
#include "borrower.h"
#include "tellico_kernel.h"
#include "latin1literal.h"
#include "tellico_debug.h"

#include <kmessagebox.h>
#include <klocale.h>

#include <qregexp.h>

using Tellico::Data::Document;
Document* Document::s_self = 0;

Document::Document()
    : QObject(), m_coll(0), m_isModified(false), m_loadAllImages(false), m_validFile(false) {
  newDocument(Collection::Book);
}

void Document::slotSetModified(bool m_/*=true*/) {
  m_isModified = m_;
  emit signalModified(m_isModified);
}

void Document::slotDocumentRestored() {
  slotSetModified(false);
}

bool Document::newDocument(int type_) {
//  kdDebug() << "Document::newDocument()" << endl;
  deleteContents();

  m_coll = CollectionFactory::collection(static_cast<Collection::Type>(type_), true);

  Kernel::self()->resetHistory();
  Controller::self()->slotCollectionAdded(m_coll);

  slotSetModified(false);
  KURL url;
  url.setFileName(i18n("Untitled"));
  setURL(url);
  m_validFile = false;

  return true;
}

bool Document::openDocument(const KURL& url_) {
//  myDebug() << "Document::openDocument() - " << url_.prettyURL() << endl;

  // delayed image loading only works for local files
  if(!url_.isLocalFile()) {
    m_loadAllImages = true;
  }

  Import::TellicoImporter importer(url_, m_loadAllImages);
  connect(&importer, SIGNAL(signalFractionDone(float)), SIGNAL(signalFractionDone(float)));

  Collection* coll = importer.collection();
  // delayed image loading only works for zip files
  // format is only known AFTER collection() is called
  if(importer.format() != Import::TellicoImporter::Zip) {
    m_loadAllImages = true;
  }
  if(!coll) {
    Kernel::self()->sorry(importer.statusMessage());
    m_validFile = false;
//    myDebug() << "Document::openDocument() - returning false" << endl;
    return false;
  }
  deleteContents();
  m_coll = coll;
  setURL(url_);
  m_validFile = true;

  Kernel::self()->resetHistory();
  Controller::self()->slotCollectionAdded(m_coll);

  slotSetModified(importer.modifiedOriginal());
  return true;
}

bool Document::saveModified() {
  bool completed = true;

  if(m_isModified) {
    MainWindow* app = static_cast<MainWindow*>(Kernel::self()->widget());
    QString str = i18n("The current file has been modified.\n"
                       "Do you want to save it?");
    int want_save = KMessageBox::warningYesNoCancel(Kernel::self()->widget(), str, i18n("Unsaved Changes"),
                                                    KStdGuiItem::save(), KStdGuiItem::discard());
    switch(want_save) {
      case KMessageBox::Yes:
        completed = app->fileSave();
        break;

      case KMessageBox::No:
        slotSetModified(false);
        completed = true;
        break;

      case KMessageBox::Cancel:
      default:
        completed = false;
        break;
    }
  }

  return completed;
}

bool Document::saveDocument(const KURL& url_) {
  // will always save as zip file, no matter if has images or not
  Export::TellicoZipExporter exporter;
  exporter.setEntries(m_coll->entries());
  exporter.setURL(url_);
  bool success = exporter.exec();

#ifndef NDEBUG
  if(!success) {
    kdDebug() << "Document::saveDocument() - not successful saving to " << url_.prettyURL() << endl;
  }
#endif

  if(success) {
    Kernel::self()->resetHistory();
    setURL(url_);
    // if successful, doc is no longer modified
    slotSetModified(false);
  }
  return success;
}

bool Document::closeDocument() {
  deleteContents();
  return true;
}

void Document::deleteContents() {
  if(m_coll) {
    Controller::self()->slotCollectionDeleted(m_coll);
  }
  m_coll = 0; // old collection gets deleted as a KSharedPtr
}

void Document::appendCollection(Collection* coll_) {
  if(!coll_) {
    return;
  }

  m_coll->blockSignals(true);
  Data::FieldVec fields = coll_->fields();
  for(FieldVec::Iterator field = fields.begin(); field != fields.end(); ++field) {
    m_coll->mergeField(field);
  }

  EntryVec entries = coll_->entries();
  for(EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    entry->setCollection(m_coll);
    m_coll->addEntry(entry);
  }
  // TODO: merge filters and loans
  m_coll->blockSignals(false);
}

Tellico::Data::MergePair Document::mergeCollection(Collection* coll_) {
  MergePair pair;
  if(!coll_) {
    return pair;
  }

  m_coll->blockSignals(true);
  Data::FieldVec fields = coll_->fields();
  for(FieldVec::Iterator field = fields.begin(); field != fields.end(); ++field) {
    m_coll->mergeField(field);
  }

  // FIXME: find a faster way than one-to-one comparison
  // music collection get a special case because of the audio file metadata might
  // be read on a per-track basis, allow merging of entries with identical titles and artists
  bool isMusic = (m_coll->type() == Collection::Album);
  const QString sep = QString::fromLatin1("; ");
  const QString artist = QString::fromLatin1("artist");
  const QString track = QString::fromLatin1("track");

  bool matches = false;
  EntryVec currEntries = m_coll->entries();
  EntryVec::Iterator currIt = currEntries.begin();
  EntryVec entries = coll_->entries();
  for(EntryVec::Iterator newIt = entries.begin(); newIt != entries.end(); ++newIt) {
    matches = false;
    for(currIt = currEntries.begin(); currIt != currEntries.end(); ++currIt) {
      if(isMusic && !newIt->title().isEmpty() && !newIt->field(artist).isEmpty()
          && newIt->title() == currIt->title() && newIt->field(artist) == currIt->field(artist)) {
        matches = true;
        // trackList 1 is collection to be merged, trackList2 is current collection
        QStringList newTracks = Data::Field::split(newIt->field(track), true);
        QStringList currTracks = Data::Field::split(currIt->field(track), true);
        for(uint idx = 0; idx < newTracks.count(); ++idx) {
          // only add track if current track id is empty or shorter
          if(!newTracks[idx].isEmpty() && (idx >= currTracks.count() || currTracks[idx].isEmpty())) {
            while(currTracks.count() <= idx) {
              currTracks += QString::null;
            }
            currTracks[idx] = newTracks[idx];
          }
        }
        // the merge item is the entry pointer and the old track value
        pair.second.append(qMakePair(EntryPtr(currIt), currIt->field(track)));
        currIt->setField(track, currTracks.join(sep));
      } else if(*currIt == *newIt) {
        matches = true;
        break;
      }
    }
    if(!matches) {
      newIt->setCollection(m_coll);
      m_coll->addEntry(newIt);
      // keep track of which entries got added
      pair.first.append(newIt);
    }
  }
  // TODO: merge filters and loans
  m_coll->blockSignals(false);
  return pair;
}

void Document::replaceCollection(Collection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "Document::replaceCollection()" << endl;

  KURL url;
  url.setFileName(i18n("Untitled"));
  setURL(url);
  m_validFile = false;

  m_coll = coll_;
  // CollectionCommand takes care of calling Controller signals
}

void Document::unAppendCollection(Collection* coll_, FieldVec origFields_) {
  if(!coll_) {
    return;
  }

  m_coll->blockSignals(true);

  QStringList origFieldNames;
  for(FieldVec::Iterator field = origFields_.begin(); field != origFields_.end(); ++field) {
    m_coll->modifyField(field);
    origFieldNames << field->name();
  }

  EntryVec entries = coll_->entries();
  for(EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    // probably don't need to do this, but on the safe side...
    entry->setCollection(coll_);
    m_coll->removeEntry(entry);
  }

  // since Collection::removeField() iterates over all entries to reset the value of the field
  // don't removeField() until after removeEntry() is done
  FieldVec currFields = m_coll->fields();
  for(FieldVec::Iterator field = currFields.begin(); field != currFields.end(); ++field) {
    if(origFieldNames.findIndex(field->name()) == -1) {
      m_coll->removeField(field);
    }
  }
  m_coll->blockSignals(false);
}

void Document::unMergeCollection(Collection* coll_, FieldVec origFields_, MergePair entryPair_) {
  if(!coll_) {
    return;
  }

  m_coll->blockSignals(true);

  QStringList origFieldNames;
  for(FieldVec::Iterator field = origFields_.begin(); field != origFields_.end(); ++field) {
    m_coll->modifyField(field);
    origFieldNames << field->name();
  }

  // first item in pair are the entries added by the operation, remove them
  EntryVec entries = entryPair_.first;
  for(EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    m_coll->removeEntry(entry);
  }

  // second item in pair are the entries which got modified by the original merge command
  const QString track = QString::fromLatin1("track");
  PairVector trackChanges = entryPair_.second;
  // need to go through them in reverse since one entry may have been modified multiple times
  // first item in the pair is the entry pointer
  // second item is the old value of the track field
  for(int i = trackChanges.count()-1; i >= 0; --i) {
    trackChanges[i].first->setField(track, trackChanges[i].second);
  }


  // since Collection::removeField() iterates over all entries to reset the value of the field
  // don't removeField() until after removeEntry() is done
  FieldVec currFields = m_coll->fields();
  for(FieldVec::Iterator field = currFields.begin(); field != currFields.end(); ++field) {
    if(origFieldNames.findIndex(field->name()) == -1) {
      m_coll->removeField(field);
    }
  }
  m_coll->blockSignals(false);
}

bool Document::isEmpty() const {
  //an empty doc may contain a collection, but no entries
  return (!m_coll || m_coll->entries().isEmpty());
}

bool Document::loadImage(const QString& id_) {
//  kdDebug() << "Document::loadImage() - id = " << id_ << endl;
  if(!m_coll || !m_validFile) {
    return false;
  }
  if(m_loadAllImages) {
#ifndef NDEBUG
    myDebug() << "Document::loadImage() - all valid images should already be loaded!" << endl;
#endif
    return false;
  }
  return Import::TellicoImporter::loadImage(m_url, id_);
}

Tellico::Data::EntryVec Document::filteredEntries(const Filter* filter_) const {
  Data::EntryVec matches;
  Data::EntryVec entries = m_coll->entries();
  for(EntryVec::Iterator it = entries.begin(); it != entries.end(); ++it) {
    if(filter_->matches(it)) {
      matches.append(it);
    }
  }
  return matches;
}

void Document::checkOutEntry(Data::Entry* entry_) {
  if(!entry_) {
    return;
  }

  const QString loaned = QString::fromLatin1("loaned");
  if(!m_coll->fieldByName(loaned)) {
    Field* f = new Field(loaned, i18n("Loaned"), Field::Bool);
    f->setFlags(Field::AllowGrouped);
    f->setCategory(i18n("Personal"));
    m_coll->addField(f);
  }
  entry_->setField(loaned, QString::fromLatin1("true"));
  m_coll->updateDicts(entry_);
}

void Document::checkInEntry(Data::Entry* entry_) {
  if(!entry_) {
    return;
  }

  const QString loaned = QString::fromLatin1("loaned");
  if(!m_coll->fieldByName(loaned)) {
    return;
  }
  entry_->setField(loaned, QString::null);
  m_coll->updateDicts(entry_);
}

void Document::renameCollection(const QString& newTitle_) {
  m_coll->setTitle(newTitle_);
}

#include "document.moc"
