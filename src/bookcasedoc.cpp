/***************************************************************************
                               bookcasedoc.cpp
                             -------------------
    begin                : Sun Sep 9 2001
    copyright            : (C) 2001, 2002, 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bookcasedoc.h"
#include "bookcase.h"
#include "bcunititem.h" // needed for searching
#include "bccollectionfactory.h"
#include "translators/bookcasexmlimporter.h"
#include "translators/bookcasexmlexporter.h"
#include "bcfilehandler.h"
#include "collections/bibtexcollection.h" // needed for Bibtex conversion

#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
#ifdef KDE_IS_VERSION
#if KDE_IS_VERSION(3,1,90)
#include <kinputdialog.h>
#else
#include <klineeditdlg.h>
#endif
#else
#include <klineeditdlg.h>
#endif

#include <qwidget.h>
#include <qstring.h>
#include <qregexp.h>

BookcaseDoc::BookcaseDoc(Bookcase* parent_, const char* name_/*=0*/)
    : QObject(parent_, name_),
      m_coll(0),
      m_isModified(false) {
  newDocument(BCCollection::Book);
}

BookcaseDoc::~BookcaseDoc() {
  delete m_coll;
  m_coll = 0;
}

void BookcaseDoc::slotSetModified(bool m_/*=true*/) {
  m_isModified = m_;
  if(m_) {
    emit signalModified();
  }
}

bool BookcaseDoc::isModified() const {
  return m_isModified;
}

void BookcaseDoc::setURL(const KURL& url_) {
  m_url = url_;
}

const KURL& BookcaseDoc::URL() const {
  return m_url;
}

bool BookcaseDoc::newDocument(int type_) {
//  kdDebug() << "BookcaseDoc::newDocument()" << endl;
  deleteContents();

  m_coll = BCCollectionFactory::collection(static_cast<BCCollection::CollectionType>(type_), true);

  emit signalCollectionAdded(m_coll);
  slotSetModified(false);
  KURL url;
  url.setFileName(i18n("Untitled"));
  m_url = url;

  return true;
}

bool BookcaseDoc::openDocument(const KURL& url_) {
  Bookcase* bookcase = static_cast<Bookcase*>(parent());
  BookcaseXMLImporter importer(url_);
  connect(&importer, SIGNAL(signalFractionDone(float)),
          bookcase, SLOT(slotUpdateFractionDone(float)));

  BCCollection* coll = importer.collection();
  if(!coll) {
    KMessageBox::sorry(bookcase, importer.statusMessage());
    return false;
  }

  QString msg = importer.statusMessage();
  if(!msg.isEmpty()) {
    KMessageBox::information(bookcase, msg);
  }

  replaceCollection(coll);
  // replaceCollection() sets the URL to Unknown
  // be sure to have this call after it
  setURL(url_);
  slotSetModified(false);
  return true;
}

bool BookcaseDoc::saveModified() {
  bool completed = true;

  if(isModified()) {
    Bookcase* app = static_cast<Bookcase*>(parent());
    QString str = i18n("The current file has been modified.\n"
                       "Do you want to save it?");
    int want_save = KMessageBox::warningYesNoCancel(app, str, i18n("Warning!"),
                                                    KStdGuiItem::save(),
                                                    KStdGuiItem::discard());
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
        slotSetModified(false);
        deleteContents();
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

bool BookcaseDoc::saveDocument(const KURL& url_) {
  BookcaseXMLExporter exporter(m_coll, m_coll->unitList());
  QDomDocument dom = exporter.exportXML(false, true);

  // last parameter means write in UTF-8
  bool success = BCFileHandler::writeURL(url_, dom.toString(), false);

  if(success) {
    setURL(url_);
    // if successful, doc is no longer modified
    slotSetModified(false);
  }
  return success;
}

bool BookcaseDoc::closeDocument() {
  deleteContents();
  return true;
}

void BookcaseDoc::deleteContents() {
  deleteCollection(m_coll);
}

BCCollection* BookcaseDoc::collection() {
  return m_coll;
}

void BookcaseDoc::addCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

  delete m_coll;
  m_coll = coll_;
  emit signalCollectionAdded(coll_);

  slotSetModified(true);
}

void BookcaseDoc::deleteCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

  if(coll_ != m_coll) {
    kdDebug() << "BookcaseDoc::slotDeleteCollection() - pointers don't match" << endl;
  }
//  kdDebug() << "BookcaseDoc::slotDeleteCollection() - deleting " << coll_->title() << endl;
  emit signalCollectionDeleted(m_coll);
  delete m_coll;
  m_coll = 0;

  slotSetModified(true);
}

void BookcaseDoc::replaceCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "BookcaseDoc::replaceCollection()" << endl;
  if(m_coll) {
    deleteCollection(m_coll);
  }

  KURL url;
  url.setFileName(i18n("Untitled"));
  setURL(url);

  m_coll = coll_;
  emit signalCollectionAdded(m_coll);

  slotSetModified(true);
}

void BookcaseDoc::appendCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

  if(coll_->collectionType() != m_coll->collectionType()) {
    kdWarning() << "BookcaseDoc::appendCollection() - appended collections must "
                   "be the same type!" << endl;
    return;
  }

  m_coll->blockSignals(true);
  BCAttributeListIterator attIt(coll_->attributeList());
  for( ; attIt.current(); ++attIt) {
    if(m_coll->attributeByName(attIt.current()->name()) == 0) {
      // does not exist in current collection, add it
      m_coll->addAttribute(attIt.current()->clone());
    }
  }

  BCUnitListIterator unitIt(coll_->unitList());
  for( ; unitIt.current(); ++unitIt) {
    // use constructor for different collection
    m_coll->addUnit(new BCUnit(*unitIt.current(), m_coll));
  }
  m_coll->blockSignals(false);
  // easiest thing is to signal collection deleted, then added?
  emit signalCollectionDeleted(m_coll);
  emit signalCollectionAdded(m_coll);

  slotSetModified(true);
}

void BookcaseDoc::slotConvertToBibtex() {
  if(m_coll->collectionType() != BCCollection::Book) {
    return;
  }
  BibtexCollection* coll = BibtexCollection::convertBookCollection(m_coll);
  replaceCollection(coll);
}

void BookcaseDoc::slotSaveUnit(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

  if(!unit_->isOwned()) {
    slotAddUnit(unit_);
    return;
  }

//  kdDebug() << "BookcaseDoc::slotSaveUnit() - modifying an existing unit." << endl;
  unit_->collection()->updateDicts(unit_);
  emit signalUnitModified(unit_);

  slotSetModified(true);
}

void BookcaseDoc::slotAddUnit(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  emit signalStatusMsg(i18n("Adding a new %1...").arg(unit_->collection()->unitTitle()));

  unit_->collection()->addUnit(unit_);
  emit signalUnitAdded(unit_);
  slotSetModified(true);
}

void BookcaseDoc::slotDeleteUnit(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  emit signalStatusMsg(i18n("Deleting the %1...").arg(unit_->collection()->unitTitle()));

  // must emit the signal before the unit is deleted since otherwise, the pointer is null
  emit signalUnitDeleted(unit_);
  bool deleted = unit_->collection()->deleteUnit(unit_);

  if(deleted) {
    slotSetModified(true);
  } else {
    // revert the signal???
    emit signalUnitAdded(unit_);
    slotSetModified(false);
  }
}

void BookcaseDoc::slotRenameCollection() {
  bool ok;
#ifdef KDE_IS_VERSION
#if KDE_IS_VERSION(3,1,90)
  QString newName = KInputDialog::getText(i18n("Rename Collection"), i18n("New Collection Name"),
                    m_coll->title(), &ok, dynamic_cast<QWidget*>(parent()));
#else
  QString newName = KLineEditDlg::getText(i18n("Rename Collection"), i18n("New Collection Name"),
                    m_coll->title(), &ok, dynamic_cast<QWidget*>(parent()));
#endif
#else
  QString newName = KLineEditDlg::getText(i18n("Rename Collection"), i18n("New Collection Name"),
                    m_coll->title(), &ok, dynamic_cast<QWidget*>(parent()));
#endif
  if(ok) {
    m_coll->setTitle(newName);
    slotSetModified(true);
    emit signalCollectionRenamed(newName);
  }
}

bool BookcaseDoc::isEmpty() const {
  //an empty doc may contain a collection, but no units
  return (m_coll == 0 || m_coll->unitList().isEmpty());
}

void BookcaseDoc::search(const QString& text_, const QString& attTitle_, int options_) {
//  kdDebug() << "BookcaseDoc::search() - looking for " << text_ << " in " << attTitle_ << endl;
  Bookcase* app = static_cast<Bookcase*>(parent());
  BCUnitItem* item = app->selectedOrFirstItem();
  if(!item) {
//    kdDebug() << "BookcaseDoc::search() - empty document" << endl;
    // doc has no items
    return;
  }

  bool searchAll     = (options_ & AllAttributes);
  bool backwards     = (options_ & FindBackwards);
  bool asRegExp      = (options_ & AsRegExp);
  bool fromBeginning = (options_ & FromBeginning);
  bool caseSensitive = (options_ & CaseSensitive);

  BCAttribute* att = 0;
  BCCollection* coll = 0;

  // if fromBeginning is used, then take the first one
  if(fromBeginning) {
    // if backwards and beginning, start at end, this is slow to traverse
    if(backwards) {
      item = dynamic_cast<BCUnitItem*>(item->listView()->lastItem());
    } else {
      item = dynamic_cast<BCUnitItem*>(item->listView()->firstChild());
    }
  } else {
    // don't want to continually search the same one, so if the returned item
    // is the same as the selected one, then skip to the next
    while(item && item->isSelected()) {
      // there is no QListViewItem::prevSibling()
      // itemABove() works since I know there are no parents in the detailed view
      if(backwards) {
        item = dynamic_cast<BCUnitItem*>(item->itemAbove());
      } else {
        item = dynamic_cast<BCUnitItem*>(item->nextSibling());
      }
    }
  }

  BCAttributeList empty;
  BCAttributeListIterator attIt(empty);

  bool found = false;
  QString matchedText;
  BCUnit* unit;
  while(item) {
    unit = item->unit();
//    kdDebug() << "\tsearching " << unit->title() << endl;;

    // if there's no current collection, or the collection has changed, update
    // the pointer and the attribute pointer and iterator
    if(!coll || coll != unit->collection()) {
      coll = unit->collection();
      if(searchAll) {
        attIt = BCAttributeListIterator(coll->attributeList());
      } else {
        att = coll->attributeByTitle(attTitle_);
      }
    }

    // reset if we're searching all
    if(searchAll) {
      att = attIt.toFirst();
    }
    // if we're not searching all, then we break out
    // if we are searching all, then att will finally be 0 when the iterator gets to the end
    while(att && !found) {
//      kdDebug() << "\t\tsearching " << att->title() << endl;
      // if RegExp is used, then the text is a regexp pattern
      if(asRegExp) {
        QRegExp rx(text_);
        if(caseSensitive) {
          rx.setCaseSensitive(true);
        }
        if(rx.search(unit->attribute(att->name())) > -1) {
          found = true;
          matchedText = rx.capturedTexts().first();
        }
      // else if not a regexp
      } else {
        if(caseSensitive) {
          if(unit->attribute(att->name()).contains(text_)) {
            found = true;
            matchedText = text_;
          }
        } else {
          // we're not case sensitive so compare lower-case to lower-case
          if(unit->attribute(att->name()).lower().contains(text_.lower())) {
            found = true;
            matchedText = text_.lower();
          }
        }
      } // end of while(att ...

      // if a unit is found, emit selected signal and return
      if(found) {
//        kdDebug() << "\tfound " << unit->attribute(att->name()) << endl;
        emit signalUnitSelected(unit, matchedText);
        return;      
      }

      // if not, then continue the search. If we're searching all, update the pointer,
      // otherwise break out and go to next item
      if(searchAll) {
        ++attIt;
        att = attIt.current();
      } else {
        break;
      }
    }
        
    // get next item
    if(backwards) {
      // there is no QListViewItem::prevSibling()
      // itemABove() works since I know there are no parents in the detailed view
      item = dynamic_cast<BCUnitItem*>(item->itemAbove());
    } else {
      item = dynamic_cast<BCUnitItem*>(item->nextSibling());
    }
  }

  // if this point is reached, no match was found
  KMessageBox::information(app, i18n("Search string '%1' not found.").arg(text_));
}
