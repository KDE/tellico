/***************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
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
#include "mainwindow.h" // needed for loading progress
#include "collectionfactory.h"
#include "translators/tellicoimporter.h"
#include "translators/tellicozipexporter.h"
#include "filehandler.h"
#include "controller.h"
#include "tellico_kernel.h"

#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdeversion.h>
#if KDE_IS_VERSION(3,1,90)
#include <kinputdialog.h>
#else
#include <klineeditdlg.h>
#endif

#include <qregexp.h>

using Tellico::Data::Document;

Document::Document(QObject* parent_, const char* name_/*=0*/)
    : QObject(parent_, name_), m_coll(0), m_isModified(false) {
  newDocument(Collection::Book);
}

Document::~Document() {
  delete m_coll;
  m_coll = 0;
}

void Document::slotSetModified(bool m_/*=true*/) {
  m_isModified = m_;
  if(m_) {
    emit signalModified();
  }
}

bool Document::newDocument(int type_) {
//  kdDebug() << "Document::newDocument()" << endl;
  deleteContents();

  m_coll = CollectionFactory::collection(static_cast<Collection::Type>(type_), true);

  slotSetModified(false);
  KURL url;
  url.setFileName(i18n("Untitled"));
  m_url = url;

  return true;
}

bool Document::openDocument(const KURL& url_) {
  MainWindow* mainwindow = static_cast<MainWindow*>(Kernel::self()->widget());

  Import::TellicoImporter importer(url_);
  connect(&importer, SIGNAL(signalFractionDone(float)),
          mainwindow, SLOT(slotUpdateFractionDone(float)));

  Collection* coll = importer.collection();
  if(!coll) {
    if(!importer.statusMessage().isEmpty()) {
      KMessageBox::sorry(mainwindow, importer.statusMessage());
    }
    return false;
  }

  replaceCollection(coll);
  // replaceCollection() sets the URL to Unknown
  // be sure to have this call after it
  setURL(url_);
  slotSetModified(false);
  return true;
}

bool Document::saveModified() {
  bool completed = true;

  if(m_isModified) {
    MainWindow* app = dynamic_cast<MainWindow*>(Kernel::self()->widget());
    QString str = i18n("The current file has been modified.\n"
                       "Do you want to save it?");
    int want_save = KMessageBox::warningYesNoCancel(app, str, i18n("Warning!"),
                                                    KStdGuiItem::save(), KStdGuiItem::discard());
    switch(want_save) {
      case KMessageBox::Yes:
        completed = app->fileSave();
        if(completed) {
          deleteContents();
        }
        break;

      case KMessageBox::No:
        deleteContents();
        slotSetModified(false); // deleteContents() sets modified to true
        completed = true;
        break;

      case KMessageBox::Cancel:
        completed = false;
        break;

      default:
        completed = false;
        break;
    }
  }

  return completed;
}

bool Document::saveDocument(const KURL& url_) {
  // will always save as zip file, no matter if has images or not
  Export::TellicoZipExporter exporter(m_coll);
  exporter.setEntryList(m_coll->entryList());
  QByteArray data = exporter.data(false);
  bool success = Tellico::FileHandler::writeDataURL(url_, data);

#ifndef NDEBUG
  if(!success) {
    kdDebug() << "Document::saveDocument() - not successful saving to " << url_.prettyURL() << endl;
  }
#endif

  if(success) {
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
  deleteCollection(m_coll);
}

void Document::addCollection(Collection* coll_) {
  if(!coll_) {
    return;
  }

  delete m_coll;
  m_coll = coll_;
  Controller::self()->slotCollectionAdded(coll_);

  slotSetModified(true);
}

void Document::deleteCollection(Collection* coll_) {
  if(!coll_) {
    return;
  }

  if(coll_ != m_coll) {
    kdDebug() << "Document::slotDeleteCollection() - pointers don't match" << endl;
  }
//  kdDebug() << "Document::slotDeleteCollection() - deleting " << coll_->title() << endl;
  Controller::self()->slotCollectionDeleted(m_coll);
  delete m_coll;
  m_coll = 0;

  slotSetModified(true);
}

void Document::replaceCollection(Collection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "Document::replaceCollection()" << endl;
  if(m_coll) {
    deleteCollection(m_coll);
  }

  KURL url;
  url.setFileName(i18n("Untitled"));
  setURL(url);

  m_coll = coll_;
  Controller::self()->slotCollectionAdded(m_coll);

  slotSetModified(true);
}

void Document::appendCollection(Collection* coll_) {
  if(!coll_) {
    return;
  }

  // only append if match, but special case importing books into bibliographies
  if(coll_->type() != m_coll->type()
     || (m_coll->type() == Collection::Bibtex && coll_->type() == Collection::Book)) {
    kdWarning() << "Document::appendCollection() - appended collections must "
                   "be the same type!" << endl;
    return;
  }

  m_coll->blockSignals(true);
  for(FieldListIterator fIt(coll_->fieldList()); fIt.current(); ++fIt) {
    m_coll->mergeField(fIt.current());
  }

  for(EntryListIterator entryIt(coll_->entryList()); entryIt.current(); ++entryIt) {
    // use constructor for different collection
    m_coll->addEntry(new Entry(*entryIt.current(), m_coll));
  }
  m_coll->blockSignals(false);
  // easiest thing is to signal collection deleted, then added?
  // FIXME: fixme Signals for delete collection and then added are yucky
  Controller::self()->slotCollectionDeleted(m_coll);
  Controller::self()->slotCollectionAdded(m_coll);

  slotSetModified(true);
}

void Document::mergeCollection(Collection* coll_) {
  if(!coll_) {
    return;
  }

  // only append if match, but special case importing books into bibliographies
  if(coll_->type() != m_coll->type()
     && (m_coll->type() != Collection::Bibtex || coll_->type() != Collection::Book)) {
    kdWarning() << "Document::mergeCollection() - merged collections must "
                   "be the same type!" << endl;
    return;
  }

  m_coll->blockSignals(true);
  for(FieldListIterator fIt(coll_->fieldList()); fIt.current(); ++fIt) {
    m_coll->mergeField(fIt.current());
  }

  // FIXME: find a faster way than one-to-one comparison
  // music collection get a special case because of the audio file metadata might
  // be read on a per-track basis, allow merging of entries with identical titles and artists
  bool isMusic = (m_coll->type() == Collection::Album);
  static const QString& sep = KGlobal::staticQString("; ");
  static const QString& artist = KGlobal::staticQString("artist");
  static const QString& track = KGlobal::staticQString("track");

  EntryListIterator currIt(m_coll->entryList());
  for(EntryListIterator newIt(coll_->entryList()); newIt.current(); ++newIt) {
    bool matches = false;
    for(currIt.toFirst(); currIt.current(); ++currIt) {
      if(isMusic && !newIt.current()->title().isEmpty() && !newIt.current()->field(artist).isEmpty()
                 && newIt.current()->title() == currIt.current()->title()
                 && newIt.current()->field(artist) == currIt.current()->field(artist)) {
        matches = true;
        // trackList 1 is collection to be merged, trackList2 is current collection
        QStringList newTracks = Data::Field::split(newIt.current()->field(track), true);
        QStringList currTracks = Data::Field::split(currIt.current()->field(track), true);
        for(uint idx = 0; idx < newTracks.count(); ++idx) {
          // only add track if current track id is empty or shorter
          if(!newTracks[idx].isEmpty() && (idx >= currTracks.count() || currTracks[idx].isEmpty())) {
            while(currTracks.count() <= idx) {
              currTracks += QString::null;
            }
            currTracks[idx] = newTracks[idx];
          }
        }
        currIt.current()->setField(track, currTracks.join(sep));
      } else if(*currIt.current() == *newIt.current()) {
        matches = true;
        break;
      }
    }
    if(!matches) {
      // use constructor for different collection
      m_coll->addEntry(new Entry(*newIt.current(), m_coll));
    }
  }
  m_coll->blockSignals(false);
  // easiest thing is to signal collection deleted, then added?
  // FIXME: fixme Signals for delete collection and then added are yucky
  Controller::self()->slotCollectionDeleted(m_coll);
  Controller::self()->slotCollectionAdded(m_coll);

  slotSetModified(true);
}

void Document::slotSaveEntries(const EntryList& list_) {
  for(EntryListIterator it(list_); it.current(); ++it) {
    saveEntry(it.current());
  }
}

void Document::saveEntry(Entry* entry_) {
  if(!entry_) {
    return;
  }

  if(!entry_->isOwned()) {
    slotAddEntry(entry_);
    return;
  }

//  kdDebug() << "Document::slotSaveEntry() - modifying an existing entry." << endl;
  entry_->collection()->updateDicts(entry_);
  Controller::self()->slotEntryModified(entry_);

  slotSetModified(true);
}

void Document::slotAddEntry(Entry* entry_) {
  if(!entry_) {
    return;
  }

  entry_->collection()->addEntry(entry_);
  Controller::self()->slotEntryAdded(entry_);
  slotSetModified(true);
}

void Document::slotDeleteEntry(Entry* entry_) {
  if(!entry_) {
    return;
  }

  // must emit the signal before the entry is deleted since otherwise, the pointer is null
  Controller::self()->slotEntryDeleted(entry_);
  bool deleted = entry_->collection()->deleteEntry(entry_);

  if(deleted) {
    slotSetModified(true);
  } else {
    // revert the signal???
    kdWarning() << "Document::slotDeleteEntry() - unsuccessful delete" << endl;
    Controller::self()->slotEntryAdded(entry_);
    slotSetModified(false);
  }
}

void Document::slotRenameCollection() {
  bool ok;
#if KDE_IS_VERSION(3,1,90)
  QString newName = KInputDialog::getText(i18n("Rename Collection"), i18n("New Collection Name"),
                    m_coll->title(), &ok, Kernel::self()->widget());
#else
  QString newName = KLineEditDlg::getText(i18n("Rename Collection"), i18n("New Collection Name"),
                    m_coll->title(), &ok, Kernel::self()->widget());
#endif
  if(ok) {
    m_coll->setTitle(newName);
    slotSetModified(true);
    Controller::self()->slotCollectionRenamed(newName);
  }
}

bool Document::isEmpty() const {
  //an empty doc may contain a collection, but no entries
  return (m_coll == 0 || m_coll->entryList().isEmpty());
}

#include "document.moc"
