/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "tellico_kernel.h"
#include "mainwindow.h"
#include "document.h"
#include "collection.h"
#include "entryitem.h"
#include "controller.h"

#include <kdebug.h>
#include <kmessagebox.h>

using Tellico::Kernel;

Kernel* Kernel::s_self = 0;

Kernel::Kernel(QObject* parent_, const char* name_/*=0*/) : QObject(parent_, name_),
    m_widget(static_cast<QWidget*>(parent_)), m_doc(new Data::Document(this)) {
}

Kernel::~Kernel() {
  delete m_doc;
  m_doc = 0;
}

Tellico::Data::Collection* Kernel::collection() {
  return m_doc->collection();
}

const KURL& Kernel::URL() {
  return m_doc->URL();
}

const QStringList& Kernel::fieldTitles() const {
  return m_doc->collection()->fieldTitles();
}

QString Kernel::fieldNameByTitle(const QString& title_) const {
  return m_doc->collection()->fieldNameByTitle(title_);
}

QString Kernel::fieldTitleByName(const QString& name_) const {
  return m_doc->collection()->fieldTitleByName(name_);
}

void Kernel::searchDocument(const QString& text_, const QString& fieldTitle_, int options_) {
//  kdDebug() << "Document::search() - looking for " << text_ << " in " << fieldTitle_ << endl;
  EntryItem* item = static_cast<MainWindow*>(m_widget)->selectedOrFirstItem();
  if(!item) {
//    kdDebug() << "Kernel::searchDocument() - empty document" << endl;
    // doc has no items
    return;
  }

  bool searchAll     = (options_ & AllFields);
  bool backwards     = (options_ & FindBackwards);
  bool asRegExp      = (options_ & AsRegExp);
  bool caseSensitive = (options_ & CaseSensitive);

  // don't want to continually search the same one, so if the returned item
  // is the same as the selected one, then skip to the next
  while(item && item->isSelected()) {
    // there is no QListViewItem::prevSibling()
    // itemAbove() works since I know there are no nested items in the detailed view
    item = static_cast<EntryItem*>(backwards ? item->itemAbove() : item->nextSibling());
  }

  // since all fields might be searched, use a list and iterator
  Data::FieldList empty;
  Data::FieldListIterator fIt(empty);

  bool found = false;
  QString matchedText;

  Data::Collection* coll = 0;
  Data::Field* field = 0;
  Data::Entry* entry = 0;
  while(item) {
    entry = item->entry();
//    kdDebug() << "\tsearching " << entry->title() << endl;;

    // if there's no current collection, or the collection has changed, update
    // the pointer and the field pointer and iterator
    if(!coll || coll != entry->collection()) {
      coll = entry->collection();
      if(searchAll) {
        fIt = Data::FieldListIterator(coll->fieldList());
      } else {
        field = coll->fieldByTitle(fieldTitle_);
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
        rx.setCaseSensitive(caseSensitive);
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
    item = static_cast<EntryItem*>(backwards ? item->itemAbove() : item->nextSibling());
  }

  // if this point is reached, no match was found
  KMessageBox::information(widget(), i18n("Search string '%1' not found.").arg(text_));
}

#include "tellico_kernel.moc"
