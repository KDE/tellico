/***************************************************************************
                               bookcasedoc.cpp
                             -------------------
    begin                : Sun Sep 9 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "bookcasedoc.h"
#include "bookcase.h"
#include "bccollection.h"

#include <kio/netaccess.h>
#include <kio/job.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <ktempfile.h>
#include <klocale.h>

#include <qwidget.h>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdom.h>

const int FORMAT_VERSION = 1;
const int OPEN_SIGNAL_STEP_SIZE = 10;
const int SAVE_SIGNAL_STEP_SIZE = 10;

BookcaseDoc::BookcaseDoc(QWidget* parent_, const char* name_/*=0*/)
 : QObject(parent_, name_), m_isModified(false) {
  m_collList.setAutoDelete(true);

  newDocument();
  setModified(false);
}

BookcaseDoc::~BookcaseDoc() {
}

void BookcaseDoc::setModified(bool m_) {
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

bool BookcaseDoc::newDocument() {
  m_isModified = false;
  m_url.setFileName(i18n("Untitled"));
  emit signalNewDoc();

  return true;
}

bool BookcaseDoc::openDocument(const KURL& url_) {
  QString tmpfile;
  if(!KIO::NetAccess::download(url_, tmpfile)) {
    Bookcase* app = static_cast<Bookcase*>(parent());
    QString str(i18n("Unable to download file - %1.").arg(url_.url()));
    KMessageBox::sorry(app, str);
    return false;
  }

  QFile f(tmpfile);
  if(!f.open(IO_ReadOnly)) {
    Bookcase* app = static_cast<Bookcase*>(parent());
    QString str(i18n("Unable to open file - %1.").arg(tmpfile));
    KMessageBox::sorry(app, str);
    KIO::NetAccess::removeTempFile(tmpfile);
    return false;
  }

  char buf[6];
  if(f.readBlock(buf, 5) < 5) {
    f.close();
    Bookcase* app = static_cast<Bookcase*>(parent());
    QString str(i18n("Unable to read file - %1.").arg(tmpfile));
    KMessageBox::sorry(app, str);
    KIO::NetAccess::removeTempFile(tmpfile);
    return false;
  }

  // Is it plain XML ?
  if(strncasecmp(buf, "<?xml", 5) != 0) {
    f.close();
    Bookcase* app = static_cast<Bookcase*>(parent());
    QString str(i18n("File is not in XML format - %1.").arg(url_.url()));
    KMessageBox::sorry(app, str);
    KIO::NetAccess::removeTempFile(tmpfile);
    return false;
  }

  f.at(0); // reset file pointer to beginning
  QDomDocument doc;
  doc.setContent(&f);
  QDomElement root = doc.documentElement();
  if(root.tagName() != "bookcase") {
    f.close();
    Bookcase* app = static_cast<Bookcase*>(parent());
    QString str(i18n("File is not a Bookcase data file - %1.").arg(url_.url()));
    KMessageBox::sorry(app, str);
    KIO::NetAccess::removeTempFile(tmpfile);
    return false;
  }

  // now the document can finally be loaded
  // TODO: a different signal should be used?
  emit signalNewDoc();
  deleteContents();
  setURL(url_);

  QDomNodeList collelems = root.elementsByTagName("collection");
  kdDebug() << QString("BookcaseDoc::openDocument() - There are %1 collection(s).\n").arg(collelems.count());

  for(unsigned i = 0; i < collelems.count(); ++i) {
    QDomElement collelem = collelems.item(i).toElement();
    QString title = collelem.attribute("title");
    QString unit = collelem.attribute("unit");
    QString unitTitle = collelem.attribute("unitTitle");
    BCCollection* coll;
    if(unit == "book") {
      coll = BCCollection::Books(collectionCount());
    } else if(unit == "cd") {
      coll = BCCollection::CDs(collectionCount());
    } else if(unit == "video") {
      coll = BCCollection::Videos(collectionCount());
    } else {
      coll = new BCCollection(collectionCount(), title, unit, unitTitle);
    }
    // since the static operators have default titles
    if(!title.isEmpty()) {
      coll->setTitle(title);
    }
    // do not do this yet, we want the collection to have all of its attributes first
    // slotAddCollection(coll);

    QDomNodeList attelems = collelem.elementsByTagName("attribute");
    kdDebug() << QString("BookcaseDoc::openDocument() - There are %1 attribute(s).\n").arg(attelems.count());

    for(unsigned j = 0; j < attelems.count(); ++j) {
      QDomElement attelem = attelems.item(j).toElement();
      BCAttribute* att = new BCAttribute(attelem.attribute("name", "unknown"),
                                         attelem.attribute("title", i18n("Unknown")));
      if(attelem.hasAttribute("allowed")) {
        att->setAllowed(QStringList::split(";", attelem.attribute("allowed")));
      }
      coll->addAttribute(att);
      kdDebug() << QString("  Added attribute: %1, %2").arg(att->name()).arg(att->title()) << endl;
    }
    // do this now that we have all the attributes
    slotAddCollection(coll);

    QDomNodeList unitelems = collelem.elementsByTagName(unit);
    kdDebug() << QString("BookcaseDoc::openDocument() - There are %1 %2(s) in the collection.").arg(unitelems.count()).arg(unit) << endl;

    for(unsigned j = 0; j < unitelems.count(); ++j) {
      BCUnit* unit = new BCUnit(coll);
      QDomNode child = unitelems.item(j).firstChild();
      for( ; !child.isNull(); child = child.nextSibling()) {
        // BCUnit::setAttribute checks to see if an attribute of 'name' is allowed
        // checkbox attributes have no text(), set it to "1"
        if(child.toElement().text().isEmpty()) {
          unit->setAttribute(child.toElement().tagName(), "1");
        } else {
          unit->setAttribute(child.toElement().tagName(), child.toElement().text());
        }
      } // end attribute list
      slotAddUnit(unit);

      // emit a signal every 5 units loaded
      float f;
      if(j%OPEN_SIGNAL_STEP_SIZE == 0) {
        // allocate equal time to each collection in document
        // i is number of collection
        // j is number of unit
        f = i+static_cast<float>(j)/static_cast<float>(unitelems.count());
        f *= 1.0/static_cast<float>(collelems.count());
        emit signalFractionDone(f);
      }
    } // end unit loop
  } // end collection loop

  // be sure to do this to properly close out progress bar
  emit signalFractionDone(1.0);

  f.close();
  KIO::NetAccess::removeTempFile(tmpfile);

  return true;
}

bool BookcaseDoc::saveModified() {
  bool completed = true;

  if(isModified()) {
    Bookcase* app = static_cast<Bookcase*>(parent());
    QString str(i18n("The current file has been modified.\n"
                     "Do you want to save it?"));
    int want_save = KMessageBox::warningYesNoCancel(app, str, i18n("Warning!"));
    switch(want_save) {
      case KMessageBox::Yes:
        if(URL().fileName() == i18n("Untitled")) {
          app->slotFileSaveAs();
        } else {
          saveDocument(URL());
        }

        deleteContents();
        completed = true;
        break;

      case KMessageBox::No:
        setModified(false);
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
  QDomDocument doc("bookcase");
  doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
  QDomElement bcelem = doc.createElement("bookcase");
  bcelem.setAttribute("version", FORMAT_VERSION);
  doc.appendChild(bcelem);

  QListIterator<BCCollection> collIt(m_collList);
  // need i counter for progress bar
  for(int i = 0; collIt.current(); ++collIt, ++i) {
    BCCollection* coll = collIt.current();
    QDomElement collElem = doc.createElement("collection");
    doc.documentElement().appendChild(collElem);
    collElem.setAttribute("title", coll->title());
    collElem.setAttribute("unit", coll->unitName());
    collElem.setAttribute("unitTitle", coll->unitTitle());

    // if the collection is custom, include the attributes in the doc file
    if(coll->isCustom()) {
      QDomElement attsElem = doc.createElement("attributes");
      collElem.appendChild(attsElem);
      QListIterator<BCAttribute> attIt(coll->attributeList());
      QDomElement attElem;
      for( ; attIt.current(); ++attIt) {
        attElem = doc.createElement("attribute");
        attsElem.appendChild(attElem);
        attElem.setAttribute("name", attIt.current()->name());
        attElem.setAttribute("title", attIt.current()->title());
        attElem.setAttribute("group", attIt.current()->group());
        attElem.setAttribute("type", attIt.current()->type());
        if(attIt.current()->type() == BCAttribute::Choice) {
          attElem.setAttribute("allowed", attIt.current()->allowed().join(";"));
        }
        attElem.setAttribute("flags", attIt.current()->flags());
      } // end attribute loop
     } // end attribute save

    QDomElement attElem;
    QDomElement unitElem;
    QListIterator<BCUnit> unitIt(coll->unitList());
    // the j counter is for progress
    for(int j = 0; unitIt.current(); ++unitIt, ++j) {
      unitElem = doc.createElement(coll->unitName());
      collElem.appendChild(unitElem);
      QListIterator<BCAttribute> attIt(coll->attributeList());
      for( ; attIt.current(); ++attIt) {
        QString attValue = unitIt.current()->attribute(attIt.current()->name());
        if(!attValue.isEmpty()) {
          attElem = doc.createElement(attIt.current()->name());
          unitElem.appendChild(attElem);
          if(attIt.current()->type() != BCAttribute::Bool) {
            attElem.appendChild(doc.createTextNode(attValue));
          }
        }
      } // end attribute loop
      attIt.toFirst();
      if(j%SAVE_SIGNAL_STEP_SIZE == 0) {
        // allocate equal time to each collection in document, leave 20% for later file write
        // i is number of collection
        // j is number of unit
        float f = i+static_cast<float>(j)/static_cast<float>(coll->unitCount());
        f *= 0.8/static_cast<float>(collectionCount());
        emit signalFractionDone(f);
      }
    } // end unit loop
  } // end collection loop

  if(KIO::NetAccess::exists(url_)) {
    KURL backup(url_);
    backup.setPath(backup.path() + QString::fromLatin1("~"));
    KIO::NetAccess::del(backup);
    KIO::NetAccess::copy(url_, backup);
  }

  bool success;
  if(url_.isLocalFile()) {
    QFile f(url_.path());
    success = writeDocument(f, doc.toCString());
    f.close();
  } else {
    KTempFile tempfile;
    QFile f(tempfile.name());
    success = writeDocument(f, doc.toCString());
    f.close();
    KIO::NetAccess::upload(tempfile.name(), url_);
    tempfile.unlink();
  }

  // now we're really done
  emit signalFractionDone(1.0);

  if(success) {
    // if successful, doc is no longer modified
    setModified(false);
  }
  return success;
}

bool BookcaseDoc::writeDocument(QFile& f_, const QCString& s_) {
  if(f_.open(IO_WriteOnly)) {
    QTextStream t(&f_);
    t << s_.data();
  } else {
    kdDebug() << "BookcaseDoc::writeDocument() - Unable to open file to write.\n";
    return false;
  }
  return true;
}

bool BookcaseDoc::closeDocument() {
  deleteContents();
  return true;
}

void BookcaseDoc::deleteContents() {
  QListIterator<BCCollection> it(m_collList);
  for( ; it.current(); ++it) {
    slotDeleteCollection(it.current());
  }
  m_collList.clear();
}

unsigned BookcaseDoc::collectionCount() const {
  return m_collList.count();
}

BCCollection* BookcaseDoc::collectionById(int id_) {
  BCCollection* coll = NULL;
  QListIterator<BCCollection> it(m_collList);
  for( ; it.current(); ++it) {
    if(it.current()->id() == id_) {
      coll = it.current();
      break;
    }
  }
  return coll;
}

const QList<BCCollection>& BookcaseDoc::collectionList() const{
  return m_collList;
}

void BookcaseDoc::slotAddCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

  emit signalCollectionAdded(coll_);
  m_collList.append(coll_);

  setModified(true);
}

void BookcaseDoc::slotDeleteCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

  emit signalCollectionDeleted(coll_);
  m_collList.remove(coll_);

  setModified(true);
}

void BookcaseDoc::slotSaveUnit(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

  if(unit_->collection()->unitList().containsRef(unit_) == 0) {
    slotAddUnit(unit_);
    return;
  }

  kdDebug() << "BookcaseDoc::slotSaveUnit() - modifying an existing unit." << endl;
  emit signalUnitModified(unit_);

  setModified(true);
}

void BookcaseDoc::slotAddUnit(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  emit signalStatusMsg(i18n("Adding a new %1...").arg(unit_->collection()->unitTitle()));

  unit_->collection()->addUnit(unit_);
  emit signalUnitAdded(unit_);
  setModified(true);
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
    setModified(true);
  } else {
    // revert the signal???
    emit signalUnitAdded(unit_);
    setModified(false);
  }
}

void BookcaseDoc::slotRenameCollection(int id_, const QString& newTitle_) {
  BCCollection* coll = collectionById(id_);
  if(coll) {
    coll->setTitle(newTitle_);
  }
  setModified(true);
}
