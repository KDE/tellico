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
#include "entryitem.h" // needed for searching
#include "collectionfactory.h"
#include "translators/bookcaseimporter.h"
#include "translators/bookcasezipexporter.h"
#include "filehandler.h"
#include "controller.h"
#include "utils.h" // needed for macro expansion
#include "kernel.h"

#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
#if KDE_IS_VERSION(3,1,90)
#include <kinputdialog.h>
#else
#include <klineeditdlg.h>
#endif

#include <qregexp.h>

using Bookcase::Data::Document;

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

bool Document::isModified() const {
  return m_isModified;
}

void Document::setURL(const KURL& url_) {
  m_url = url_;
}

const KURL& Document::URL() const {
  return m_url;
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
  MainWindow* bookcase = dynamic_cast<MainWindow*>(Kernel::self()->widget());

  Import::BookcaseImporter importer(url_);
  connect(&importer, SIGNAL(signalFractionDone(float)),
          bookcase, SLOT(slotUpdateFractionDone(float)));

  Collection* coll = importer.collection();
  if(!coll) {
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
        if(app->isNewDocument()) {
          app->slotFileSaveAs();
        } else {
          saveDocument(URL());
        }
        deleteContents();
        completed = true;
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
  Export::BookcaseZipExporter exporter(m_coll);
  exporter.setEntryList(m_coll->entryList());
  QByteArray data = exporter.data(false);
  bool success = FileHandler::writeDataURL(url_, data);

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
     || (m_coll->type() == Collection::Bibtex && coll_->type() == Collection::Book)) {
    kdWarning() << "Document::mergeCollection() - merged collections must "
                   "be the same type!" << endl;
    return;
  }

  m_coll->blockSignals(true);
  for(FieldListIterator fIt(coll_->fieldList()); fIt.current(); ++fIt) {
    m_coll->mergeField(fIt.current());
  }

  // FIXME: find a faster way than one-to-one comparison
  for(EntryListIterator it(coll_->entryList()); it.current(); ++it) {
    bool matches = false;
    for(EntryListIterator it2(m_coll->entryList()); it2.current(); ++it2) {
      if(*it2.current() == *it.current()) {
        matches = true;
        break;
      }
    }
    if(!matches) {
      // use constructor for different collection
      m_coll->addEntry(new Entry(*it.current(), m_coll));
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
                    m_coll->title(), &ok, static_cast<QWidget*>(parent()));
#else
  QString newName = KLineEditDlg::getText(i18n("Rename Collection"), i18n("New Collection Name"),
                    m_coll->title(), &ok, static_cast<QWidget*>(parent()));
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

void Document::search(const QString& text_, const QString& attTitle_, int options_) {
//  kdDebug() << "Document::search() - looking for " << text_ << " in " << attTitle_ << endl;
  MainWindow* app = static_cast<MainWindow*>(parent());
  EntryItem* item = app->selectedOrFirstItem();
  if(!item) {
//    kdDebug() << "Document::search() - empty document" << endl;
    // doc has no items
    return;
  }

  bool searchAll     = (options_ & AllFields);
  bool backwards     = (options_ & FindBackwards);
  bool asRegExp      = (options_ & AsRegExp);
  bool fromBeginning = (options_ & FromBeginning);
  bool caseSensitive = (options_ & CaseSensitive);

  Field* field = 0;
  Collection* coll = 0;

  // if fromBeginning is used, then take the first one
  if(fromBeginning) {
    // if backwards and beginning, start at end, this is slow to traverse
    if(backwards) {
      item = static_cast<EntryItem*>(item->listView()->lastItem());
    } else {
      item = static_cast<EntryItem*>(item->listView()->firstChild());
    }
  } else {
    // don't want to continually search the same one, so if the returned item
    // is the same as the selected one, then skip to the next
    while(item && item->isSelected()) {
      // there is no QListViewItem::prevSibling()
      // itemABove() works since I know there are no parents in the detailed view
      if(backwards) {
        item = static_cast<EntryItem*>(item->itemAbove());
      } else {
        item = static_cast<EntryItem*>(item->nextSibling());
      }
    }
  }

  FieldList empty;
  FieldListIterator fIt(empty);

  bool found = false;
  QString matchedText;
  Entry* entry;
  while(item) {
    entry = item->entry();
//    kdDebug() << "\tsearching " << entry->title() << endl;;

    // if there's no current collection, or the collection has changed, update
    // the pointer and the field pointer and iterator
    if(!coll || coll != entry->collection()) {
      coll = entry->collection();
      if(searchAll) {
        fIt = FieldListIterator(coll->fieldList());
      } else {
        field = coll->fieldByTitle(attTitle_);
      }
    }

    // reset if we're searching all
    if(searchAll) {
      field = fIt.toFirst();
    }
    // if we're not searching all, then we break out
    // if we are searching all, then field will finally be 0 when the iterator gets to the end
    while(field && !found) {
//      kdDebug() << "\t\tsearching " << field->title() << endl;
      // if RegExp is used, then the text is a regexp pattern
      if(asRegExp) {
        QRegExp rx(text_);
        if(caseSensitive) {
          rx.setCaseSensitive(true);
        }
        if(rx.search(entry->field(field->name())) > -1) {
          found = true;
          matchedText = rx.capturedTexts().first();
        }
      // else if not a regexp
      } else {
        if(caseSensitive) {
          if(entry->field(field->name()).contains(text_)) {
            found = true;
            matchedText = text_;
          }
        } else {
          // we're not case sensitive so compare lower-case to lower-case
          if(entry->field(field->name()).lower().contains(text_.lower())) {
            found = true;
            matchedText = text_.lower();
          }
        }
      } // end of while(field ...

      // if a entry is found, emit selected signal and return
      if(found) {
//        kdDebug() << "\tfound " << entry->field(field->name()) << endl;
        Controller::self()->slotUpdateSelection(entry, matchedText);
        return;
      }

      // if not, then continue the search. If we're searching all, update the pointer,
      // otherwise break out and go to next item
      if(searchAll) {
        ++fIt;
        field = fIt.current();
      } else {
        break;
      }
    }

    // get next item
    if(backwards) {
      // there is no QListViewItem::prevSibling()
      // itemABove() works since I know there is only one level of items int he lsitview
      item = static_cast<EntryItem*>(item->itemAbove());
    } else {
      item = static_cast<EntryItem*>(item->nextSibling());
    }
  }

  // if this point is reached, no match was found
  KMessageBox::information(app, i18n("Search string '%1' not found.").arg(text_));
}

#include "document.moc"
