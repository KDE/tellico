/***************************************************************************
                               bookcasedoc.cpp
                             -------------------
    begin                : Sun Sep 9 2001
    copyright            : (C) 2001 by Robby Stephenson
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
#include "bcunititem.h"
#include "bookcollection.h"
#include "bcutils.h"

#include <kio/netaccess.h>
#include <kio/job.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <klocale.h>

#include <qwidget.h>
#include <qfile.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qregexp.h>

/*
 * VERSION 2 added namespaces, changed to multiple elements,
 * and changed the "keywords" attribute to "keyword"
 *
 * VERSION 3 broek out the formatFlag, and change NoComplete to AllowCompletion
 */
static const int SYNTAX_VERSION = 3;

static const int OPEN_SIGNAL_STEP_SIZE = 10;
//static const int SAVE_SIGNAL_STEP_SIZE = 10;

BookcaseDoc::BookcaseDoc(QWidget* parent_, const char* name_/*=0*/)
    : QObject(parent_, name_), m_isModified(false) {
  m_collList.setAutoDelete(true);
  newDocument();
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

bool BookcaseDoc::newDocument() {
//  kdDebug() << "BookcaseDoc::newDocument()" << endl;
  deleteContents();

  // a new document always has an empty book collection
  // the 0 is the collection number
  BookCollection* coll = new BookCollection(0);

  // can't call slotAddCollection() because setModified(true) gets called
  m_collList.append(coll);
  emit signalCollectionAdded(coll);

  slotSetModified(false);
  m_url.setFileName(i18n("Untitled"));

  return true;
}

QDomDocument* BookcaseDoc::readDocument(const KURL& url_) const {
  QString tmpfile;
  if(!KIO::NetAccess::download(url_, tmpfile)) {
    Bookcase* app = BookcaseAncestor(parent());
    QString str;
    if(url_.isLocalFile()) {
      str = i18n("Bookcase is unable to find the file - %1.").arg(url_.fileName());
    } else {
      str = i18n("Bookcase is unable to download the file - %1.").arg(url_.url());
    }
    KMessageBox::sorry(app, str);
    return 0;
  }

  QFile f(tmpfile);
  if(!f.open(IO_ReadOnly)) {
    Bookcase* app = BookcaseAncestor(parent());
    QString str(i18n("Bookcase is unable to open the file - %1.").arg(tmpfile));
    KMessageBox::sorry(app, str);
    KIO::NetAccess::removeTempFile(tmpfile);
    return 0;
  }

  char buf[6];
  if(f.readBlock(buf, 5) < 5) {
    f.close();
    Bookcase* app = BookcaseAncestor(parent());
    QString str(i18n("Bookcase is unable to read the file - %1.").arg(tmpfile));
    KMessageBox::sorry(app, str);
    KIO::NetAccess::removeTempFile(tmpfile);
    return 0;
  }

  // Is it plain XML ?
  if(strncasecmp(buf, "<?xml", 5) != 0) {
    f.close();
    Bookcase* app = BookcaseAncestor(parent());
    QString str(i18n("Bookcase is unable to load the file - %1.").arg(url_.fileName()));
    KMessageBox::sorry(app, str);
    KIO::NetAccess::removeTempFile(tmpfile);
    return 0;
  }

  f.at(0); // reset file pointer to beginning
  QDomDocument* dom = new QDomDocument();
  QString errorMsg;
  int errorLine, errorColumn; 
  if(!dom->setContent(&f, false, &errorMsg, &errorLine, &errorColumn)) {
    f.close();
    Bookcase* app = BookcaseAncestor(parent());
    QString str = i18n("Bookcase is unable to load the file - %1.").arg(url_.fileName());
    QString details = i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
    details += QString::fromLatin1("\n");
    details += i18n("The error message from Qt is:");
    details += QString::fromLatin1("\n");
    details += QString::fromLatin1("\t") + errorMsg;
    KMessageBox::detailedSorry(app, str, details);
    KIO::NetAccess::removeTempFile(tmpfile);
    delete dom;
    return 0;
  }
  
  f.close();
  KIO::NetAccess::removeTempFile(tmpfile);
  return dom;
}

bool BookcaseDoc::openDocument(const KURL& url_) {
  QDomDocument* dom = readDocument(url_);
  if(!dom) {
    return false;
  }
  
  bool success = loadDomDocument(url_, *dom);
  delete dom;
  if(success) {
    slotSetModified(false);
  }
  return success;
}

bool BookcaseDoc::loadDomDocument(const KURL& url_, const QDomDocument& dom_) {  
  Bookcase* app = BookcaseAncestor(parent());
  QDomElement root = dom_.documentElement();
  if(root.tagName() != QString::fromLatin1("bookcase")) {
    QString str(i18n("Bookcase is unable to load the file - %1.").arg(url_.fileName()));
    KMessageBox::sorry(app, str);
    return false;
  }
  
  // the syntax version attribute name changed from "version" to "syntaxVersion"
  int syntaxVersion;
  if(root.hasAttribute(QString::fromLatin1("syntaxVersion"))) {
    syntaxVersion = root.attribute(QString::fromLatin1("syntaxVersion")).toInt();
  } else if (root.hasAttribute(QString::fromLatin1("version"))) {
    syntaxVersion = root.attribute(QString::fromLatin1("version")).toInt();
  } else {
    QString str(i18n("Bookcase is unable to load the file - %1.").arg(url_.fileName()));
    KMessageBox::sorry(app, str);
    return false;
  }

  if(syntaxVersion > SYNTAX_VERSION) {
    QString str(i18n("Bookcase is unable to load the file - %1.").arg(url_.fileName()));
    str += QString::fromLatin1("\n");
    str += i18n("It is from a future version of Bookcase.");
    KMessageBox::sorry(app, str);
    return false;
  } else if(syntaxVersion < SYNTAX_VERSION) {
    kdDebug() << "BookcaseDoc::loadDomDocument() - converting from syntax version "
              << syntaxVersion << " to " << SYNTAX_VERSION << endl;
  }
  
  // now the document can finally be loaded
  deleteContents();
  setURL(url_);

  QDomNodeList collelems = root.elementsByTagName(QString::fromLatin1("collection"));
  if(collelems.count() > 1) {
    kdWarning() << "BookcaseDoc::openDocument() - There is more than one collection."
                   "This isn't supported at the moment. Only the first will be loaded." << endl;
  }

// for now, don't support more than one collection
//  for(unsigned i = 0; i < collelems.count(); ++i) {
  for(unsigned i = 0; i < 1; ++i) {
    QDomElement collelem = collelems.item(i).toElement();
    QString title = collelem.attribute(QString::fromLatin1("title"));
    QString unit = collelem.attribute(QString::fromLatin1("unit"));
    QString unitTitle = collelem.attribute(QString::fromLatin1("unitTitle"));
    BCCollection* coll;
    if(unit == QString::fromLatin1("book")) {
        coll = new BookCollection(collectionCount());
#if 0
    } else if(unit == QString::fromLatin1("cd")) {
      coll = BCCollection::CDs(collectionCount());
    } else if(unit == QString::fromLatin1("video")) {
      coll = BCCollection::Videos(collectionCount());
#endif
    } else {
      coll = new BCCollection(collectionCount(), title, unit, unitTitle);
    }
    // since the static operators have default titles
    if(!title.isEmpty()) {
      coll->setTitle(title);
    }
    // do not do this yet, we want the collection to have all of its attributes first
    // slotAddCollection(coll);

    // I don't want the attribute added and unit added signals to happen
    coll->blockSignals(true);

    // there will only be attributes if it's a custom collection
    QDomNodeList attelems = collelem.elementsByTagName(QString::fromLatin1("attribute"));
//    kdDebug() << QString("BookcaseDoc::openDocument() - There are %1 attribute(s).\n").arg(attelems.count());

    QString attName, attTitle, attTypeStr, attFormatStr;
    for(unsigned j = 0; j < attelems.count(); ++j) {
      QDomElement attelem = attelems.item(j).toElement();
      
      attName  = attelem.attribute(QString::fromLatin1("name"), QString::fromLatin1("unknown"));
      attTitle = attelem.attribute(QString::fromLatin1("title"), i18n("Unknown"));

      attTypeStr = attelem.attribute(QString::fromLatin1("type"), QString::number(BCAttribute::Line));
      // is it ok to cast from an enum to an int?
      BCAttribute::AttributeType attType = static_cast<BCAttribute::AttributeType>(attTypeStr.toInt());

      BCAttribute* att;
      if(attType == BCAttribute::Choice) {
        QString attAllowed = attelem.attribute(QString::fromLatin1("allowed"));
        att = new BCAttribute(attName, attTitle, QStringList::split(QString::fromLatin1(";"),
                                                                    attAllowed));
      } else {
        att = new BCAttribute(attName, attTitle, attType);
      }
      
      if(attelem.hasAttribute(QString::fromLatin1("category"))) {
        att->setCategory(attelem.attribute(QString::fromLatin1("category")));
      }
      
      if(attelem.hasAttribute(QString::fromLatin1("flags"))) {
        int flags = attelem.attribute(QString::fromLatin1("flags")).toInt();
        att->setFlags(flags);
      }

      attFormatStr = attelem.attribute(QString::fromLatin1("format"), QString::number(BCAttribute::FormatPlain));
      BCAttribute::FormatFlag format = static_cast<BCAttribute::FormatFlag>(attFormatStr.toInt());
      att->setFormatFlag(format);
      
      if(attelem.hasAttribute(QString::fromLatin1("description"))) {
        att->setDescription(attelem.attribute(QString::fromLatin1("description")));
      }
      
      // before syntax version 3, the only custom attribute would have been bibtex-id
      if(syntaxVersion > 2) {
        coll->addAttribute(att);
      } else if(att->name() == QString::fromLatin1("bibtex-id")) {
        att->setFlags(0); // I changed the enum numbers. A no-no, I know, but I did it anyway
        coll->addAttribute(att);
      }
//      kdDebug() << QString("  Added attribute: %1, %2").arg(att->name()).arg(att->title()) << endl;
    }
    // do this now that we have all the attributes
    slotAddCollection(coll);

    QDomNodeList unitelems = collelem.elementsByTagName(unit);
//    kdDebug() << QString("BookcaseDoc::openDocument() - There are %1 %2(s) in the collection.").arg(unitelems.count()).arg(unit) << endl;

    for(unsigned j = 0; j < unitelems.count(); ++j) {
      BCUnit* unit = new BCUnit(coll);
      QDomNode attNode = unitelems.item(j).firstChild();
      for( ; !attNode.isNull(); attNode = attNode.nextSibling()) {
        // BCUnit::setAttribute checks to see if an attribute of 'name' is allowed
        // checkbox attributes have no text(), set it to "1"
        if(attNode.toElement().text().isEmpty()) {
          // "1" means checked
          unit->setAttribute(attNode.toElement().tagName(), QString::fromLatin1("1"));
        } else {
          QString attName = attNode.toElement().tagName();
          // if the first child of the attNode is a text node, just add it
          // otherwise, recurse over the attNode's children
          // this is the case for <authors><author>..</author></authors>
          if(attNode.firstChild().nodeType() == QDomNode::TextNode) {
            // in version 2, "keywords" changed to "keyword"
            if(syntaxVersion < 2 && attName == QString::fromLatin1("keywords")) {
              attName = QString::fromLatin1("keyword");
            }
            unit->setAttribute(attName, attNode.toElement().text());
          } else {
            QString attValue;
            QDomNode attChildNode = attNode.firstChild();
            for( ; !attChildNode.isNull(); attChildNode = attChildNode.nextSibling()) {
              attValue += attChildNode.toElement().text() + QString::fromLatin1("; ");
            }
            // remove the last semi-color and space
            attValue.truncate(attValue.length() - 2);
            // now the attName has the final 's', so remove it
            unit->setAttribute(attName.left(attName.length()-1), attValue);
          }
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
    coll->blockSignals(false);
    // now the groupview needs to be updated
    // this only works since the group view totally populates when grouping is changed
    app->slotChangeGrouping();
  } // end collection loop
  
  //blockSignals(false);

  // be sure to do this to properly close out progress bar
  emit signalFractionDone(1.0);
  
  return true;
}

bool BookcaseDoc::saveModified() {
  bool completed = true;

  if(isModified()) {
    Bookcase* app = BookcaseAncestor(parent());
    QString str(i18n("The current file has been modified.\n"
                     "Do you want to save it?"));
    int want_save = KMessageBox::warningYesNoCancel(app, str, i18n("Warning!"),
                                                    KStdGuiItem::save(),
                                                    KStdGuiItem::discard());
    switch(want_save) {
      case KMessageBox::Yes:
        if(m_url.fileName() == i18n("Untitled")) {
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
  QDomDocument dom = exportXML();

  bool success = writeURL(url_,  dom.toString());
  
  if(success) {
    setURL(url_);
    // if successful, doc is no longer modified
    slotSetModified(false);
  }
  return success;
}

bool BookcaseDoc::writeURL(const KURL& url_,  const QString& text_, bool locale_/*=false*/) const {
  if(KIO::NetAccess::exists(url_)) {
    if(url_ != m_url) {
      Bookcase* app = BookcaseAncestor(parent());
      QString str = i18n("A file named \"%1\" already exists. "
                         "Are you sure you want to overwrite it?").arg(url_.fileName());
      int want_continue = KMessageBox::warningContinueCancel(app, str,
                                                             i18n("Overwrite File?"),
                                                             i18n("Overwrite"));

      if(want_continue == KMessageBox::Cancel) {
        return false;
      }
    }

    KURL backup(url_);
    backup.setPath(backup.path() + QString::fromLatin1("~"));
    KIO::NetAccess::del(backup);
    KIO::NetAccess::copy(url_, backup);
  }

  bool success;
  if(url_.isLocalFile()) {
    QFile f(url_.path());
    success = writeFile(f, text_, locale_);
    f.close();
  } else {
    KTempFile tempfile;
    QFile f(tempfile.name());
    success = writeFile(f, text_, locale_);
    f.close();
    if(success) {
      bool uploaded = KIO::NetAccess::upload(tempfile.name(), url_);
      if(!uploaded) {
        Bookcase* app = BookcaseAncestor(parent());
        QString str(i18n("Bookcase is unable to upload the file - %1.").arg(url_.url()));
        KMessageBox::sorry(app, str);
      }
    }
    tempfile.unlink();
  }

  return success;
}

bool BookcaseDoc::writeFile(QFile& f_, const QString& text_, bool locale_) const {
  if(f_.open(IO_WriteOnly)) {
    QTextStream t(&f_);
    if(locale_) {
      t.setEncoding(QTextStream::Locale);
    } else {
      t.setEncoding(QTextStream::UnicodeUTF8);
    }
//    kdDebug() << "-----------------------------" << endl
//              << text_ << endl
//              << "-----------------------------" << endl;
    t << text_;
    return true;
  } else {
    Bookcase* app = BookcaseAncestor(parent());
    QString str(i18n("Bookcase is unable to write the file - %1.").arg(f_.name()));
    KMessageBox::sorry(app, str);
    return false;
  }
}

bool BookcaseDoc::closeDocument() {
  deleteContents();
  return true;
}

void BookcaseDoc::deleteContents() {
  BCCollectionListIterator it(m_collList);
  for( ; it.current(); ++it) {
    slotDeleteCollection(it.current());
  }
  m_collList.clear();
}

unsigned BookcaseDoc::collectionCount() const {
  return m_collList.count();
}

BCCollection* BookcaseDoc::collectionById(int id_) {
  BCCollection* coll = 0;
  BCCollectionListIterator it(m_collList);
  for( ; it.current(); ++it) {
    if(it.current()->id() == id_) {
      coll = it.current();
      break;
    }
  }
  if(!coll) {
    kdDebug() << "BookcaseDoc::collectionById() - no collection found for id=" << id_ << endl;
  }
  return coll;
}

const BCCollectionList& BookcaseDoc::collectionList() const{
  return m_collList;
}

QDomDocument BookcaseDoc::exportXML(bool format_/*=false*/) const {
  QDomImplementation impl;
  QDomDocumentType doctype = impl.createDocumentType(QString::fromLatin1("bookcase"),
                                                     QString::null,
                                                     QString::fromLatin1("bookcase.dtd"));
  //default namespace
  QString ns = QString::fromLatin1("http://periapsis.org/bookcase/");

  QDomDocument doc = impl.createDocument(ns, QString::fromLatin1("bookcase"), doctype);

  // root bookcase element
  QDomElement bcelem = doc.documentElement();

  // createDocument creates a root node, insert the processing instruction before it
  // Is it truly UTF_8 encoding? Yes, QDomDocument::toCString() returns UTF-8
  doc.insertBefore(doc.createProcessingInstruction(QString::fromLatin1("xml"),
                                                   QString::fromLatin1("version=\"1.0\" "
                                                                       "encoding=\"UTF-8\"")),
                                                   bcelem);

  bcelem.setAttribute(QString::fromLatin1("syntaxVersion"), SYNTAX_VERSION);

  BCCollectionListIterator collIt(m_collList);
  for( ; collIt.current(); ++collIt) {
    exportCollectionXML(doc, bcelem, collIt.current(), format_);
  }
  
//  QFile f(QString::fromLatin1("/tmp/test.xml"));
//  if(f.open(IO_WriteOnly)) {
//    QTextStream t(&f);
//    t << doc.toCString().data();
//  }
//  f.close();

  return doc;
}

QDomDocument BookcaseDoc::exportXML(const QString& dictName_, bool format_) const {
  QDomImplementation impl;
  QDomDocumentType doctype = impl.createDocumentType(QString::fromLatin1("bookcase"),
                                                     QString::null,
                                                     QString::fromLatin1("bookcase.dtd"));
  //default namespace
  QString ns = QString::fromLatin1("http://periapsis.org/bookcase/");

  QDomDocument doc = impl.createDocument(ns, QString::fromLatin1("bookcase"), doctype);
  
  // root bookcase element
  QDomElement bcelem = doc.documentElement();

  // createDocument creates a root node, insert the processing instruction before it
  // Is it truly UTF_8 encoding? Yes, QDomDocument::toCString() returns UTF-8
  doc.insertBefore(doc.createProcessingInstruction(QString::fromLatin1("xml"),
                                                   QString::fromLatin1("version=\"1.0\" "
                                                                       "encoding=\"UTF-8\"")),
                                                   bcelem);

  bcelem.setAttribute(QString::fromLatin1("syntaxVersion"), SYNTAX_VERSION);

  BCCollectionListIterator collIt(m_collList);
  for( ; collIt.current(); ++collIt) {
    QDomElement collElem = doc.createElement(QString::fromLatin1("collection"));
    bcelem.appendChild(collElem);
    collElem.setAttribute(QString::fromLatin1("title"), collIt.current()->title());

    QDomElement attsElem = doc.createElement(QString::fromLatin1("attributes"));
    collElem.appendChild(attsElem);

    // the XSLT sheet gets the header title from this
    BCAttributeListIterator attIt(collIt.current()->attributeList());
    for( ; attIt.current(); ++attIt) {
      exportAttributeXML(doc, attsElem, attIt.current());
    }

    QString value;
        
    // outside the loop for efficiency;
    QDomElement unitElem, titleElem, valueElem;

    BCUnitGroupDict* dict = collIt.current()->unitGroupDictByName(dictName_);
    if(!dict) {
      kdWarning() << "BookcaseDoc::exportXML() - no dict found for " << dictName_ << endl;
      return QDomDocument();
    }
    // dictName_ equals group->attributeName() for all these
    QDictIterator<BCUnitGroup> groupIt(*dict);
    for( ; groupIt.current(); ++groupIt) {
      BCUnitListIterator it(*groupIt.current());
      for( ; it.current(); ++it) {
        unitElem = doc.createElement(collIt.current()->unitName());

        titleElem = doc.createElement(QString::fromLatin1("title"));
        titleElem.appendChild(doc.createTextNode(it.current()->title()));
        unitElem.appendChild(titleElem);

        valueElem = doc.createElement(dictName_);
        valueElem.appendChild(doc.createTextNode(groupIt.current()->groupName()));
        unitElem.appendChild(valueElem);

        for(attIt.toFirst(); attIt.current(); ++attIt) {
          // don't include the grouped attribute twice nor the title again
          if(attIt.current()->name() == dictName_
             || attIt.current()->name() == QString::fromLatin1("title")) {
            continue;
          }

          if(format_) {
            value = it.current()->attributeFormatted(attIt.current()->name(),
                                                     attIt.current()->formatFlag());
          } else {
            value = it.current()->attribute(attIt.current()->name());
          }
          
          if(!value.isEmpty()) {
            valueElem = doc.createElement(attIt.current()->name());
            unitElem.appendChild(valueElem);
            if(attIt.current()->type() != BCAttribute::Bool) {
              valueElem.appendChild(doc.createTextNode(value));
            }
          }
        } // end attribute loop

        collElem.appendChild(unitElem);
      } // end unit loop
    } // end group loop
  } // end collection loop
  
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t << doc.toCString().data();
  }
  f.close();

  return doc;
}

void BookcaseDoc::slotAddCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

  m_collList.append(coll_);
  emit signalCollectionAdded(coll_);

  slotSetModified(true);
}

void BookcaseDoc::slotDeleteCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

  emit signalCollectionDeleted(coll_);
  m_collList.remove(coll_);

  slotSetModified(true);
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

void BookcaseDoc::slotRenameCollection(int id_, const QString& newTitle_) {
  BCCollection* coll = collectionById(id_);
  if(coll) {
    coll->setTitle(newTitle_);
    slotSetModified(true);
  }
}

bool BookcaseDoc::isEmpty() const {
  //an empty doc may contain a collection, but no units
  if(m_collList.isEmpty()) {
    return true;
  }

  BCCollectionListIterator collIt(m_collList);
  for( ; collIt.current(); ++collIt) {
    if(!collIt.current()->unitList().isEmpty()) {
      return false;
    }
  }

  return true;
}
  
QDomDocument* BookcaseDoc::importBibtex(const KURL& url_) {
  QDomDocument* dom = new QDomDocument();
  return dom;
}

void BookcaseDoc::search(const QString& text_, const QString& attTitle_, int options_) {
//  kdDebug() << "BookcaseDoc::search() - looking for " << text_ << " in " << attTitle_ << endl;
  Bookcase* app = BookcaseAncestor(parent());
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
      item = static_cast<BCUnitItem*>(item->listView()->lastItem());
    } else {
      item = static_cast<BCUnitItem*>(item->listView()->firstChild());
    }
  } else {
    // don't want to continually search the same one, so if the returned item
    // is the same as the selected one, then skip to the next
    if(item->isSelected()) {
      if(backwards) {
        item = static_cast<BCUnitItem*>(item->itemAbove());
      } else {
        item = static_cast<BCUnitItem*>(item->nextSibling());
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
      item = static_cast<BCUnitItem*>(item->itemAbove());
    } else {
      item = static_cast<BCUnitItem*>(item->nextSibling());
    }
  }

  // if this point is reached, no match was found
  KMessageBox::information(app, i18n("Search string '%1' not found.").arg(text_));
}

BCAttributeList BookcaseDoc::uniqueAttributes(int type_ /* =0 */) const {
  BCAttributeList list;
  BCCollectionListIterator it(collectionList());
  for( ; it.current(); ++it) {
    if(type_ == 0 || it.current()->collectionType() == type_) {
      if(list.isEmpty()) {
        list = it.current()->attributeList();
      } else {
        BCAttributeListIterator attIt(it.current()->attributeList());
        for( ; attIt.current(); ++attIt) {
          if(list.containsRef(attIt.current()) == 0) {
            list.append(attIt.current());
          }
        }
      }
    }
  }
  return list;
}

QString BookcaseDoc::attributeNameByTitle(const QString& title_, int type_ /* =0 */) {
  BCAttribute* att;
  BCCollectionListIterator it(collectionList());
  for( ; it.current(); ++it) {
    if(type_ == 0 || it.current()->collectionType() == type_) {
      att = it.current()->attributeByTitle(title_);
      if(att) {
        return att->name();
      }
    }
  }
  kdDebug() << "BookcaseDoc::attributeNameByTitle() - no attribute titled " << title_ << endl;
  return QString::null;
}

QString BookcaseDoc::attributeTitleByName(const QString& name_, int type_ /* =0 */) {
  BCAttribute* att;
  BCCollectionListIterator it(collectionList());
  for( ; it.current(); ++it) {
    if(type_ == 0 || it.current()->collectionType() == type_) {
      att = it.current()->attributeByName(name_);
      if(att) {
        return att->title();
      }
    }
  }
  kdDebug() << "BookcaseDoc::attributeNameByTitle() - no attribute named " << name_ << endl;
  return QString::null;
}

void BookcaseDoc::exportCollectionXML(QDomDocument& doc_, QDomElement& parent_, BCCollection* coll_, bool format_) const {
  QDomElement collElem = doc_.createElement(QString::fromLatin1("collection"));
  
  collElem.setAttribute(QString::fromLatin1("title"),     coll_->title());
  collElem.setAttribute(QString::fromLatin1("unit"),      coll_->unitName());
  collElem.setAttribute(QString::fromLatin1("unitTitle"), coll_->unitTitle());

  QDomElement attsElem = doc_.createElement(QString::fromLatin1("attributes"));
  collElem.appendChild(attsElem);
  BCAttributeListIterator attIt(coll_->attributeList());
  for( ; attIt.current(); ++attIt) {
    exportAttributeXML(doc_, attsElem, attIt.current());
  }

  // iterate over every unit in collecction
  BCUnitListIterator unitIt(coll_->unitList());

  for( ; unitIt.current(); ++unitIt) {
    exportUnitXML(doc_, collElem, unitIt.current(), format_);
  }

  parent_.appendChild(collElem);
}

void BookcaseDoc::exportAttributeXML(QDomDocument& doc_, QDomElement& parent_, BCAttribute* att_) const {
  QDomElement attElem = doc_.createElement(QString::fromLatin1("attribute"));

  attElem.setAttribute(QString::fromLatin1("name"),          att_->name());
  attElem.setAttribute(QString::fromLatin1("title"),         att_->title());
  attElem.setAttribute(QString::fromLatin1("category"),      att_->category());
  attElem.setAttribute(QString::fromLatin1("type"),          att_->type());
  attElem.setAttribute(QString::fromLatin1("flags"),         att_->flags());
  attElem.setAttribute(QString::fromLatin1("format"),        att_->formatFlag());

  if(att_->type() == BCAttribute::Choice) {
    attElem.setAttribute(QString::fromLatin1("allowed"),     att_->allowed().join(QString::fromLatin1(";")));
  }

  // only save description if it's not equal to title, which is the default
  // title is never empty, so this indirectly checks for empty descriptions
  if(att_->description() != att_->title()) {
    attElem.setAttribute(QString::fromLatin1("description"), att_->description());
  }

  parent_.appendChild(attElem);
}

void BookcaseDoc::exportUnitXML(QDomDocument& doc_, QDomElement& parent_, BCUnit* unit_, bool format_) const {
  QDomElement unitElem = doc_.createElement(unit_->collection()->unitName());

  // is it really faster to put these outside the loop?
  // parent element if attribute contains multiple values, child of unitElem
  QDomElement attParElem;
  // element for attribute value, child of eith unitElem or attParElem
  QDomElement attElem;

  // iterate through every attribute for the unit
  BCAttributeListIterator attIt(unit_->collection()->attributeList());
  for( ; attIt.current(); ++attIt) {
    QString attName = attIt.current()->name();
    QString attValue;
    if(format_) {
      attValue = unit_->attributeFormatted(attName, attIt.current()->formatFlag());
    } else {
      attValue = unit_->attribute(attName);
    }

    // if empty, then no attribute element is added and just continue
    if(attValue.isEmpty()) {
      continue;
    }

    // if multiple versions are allowed, split them into separate elements
    if(attIt.current()->flags() & BCAttribute::AllowMultiple) {
      // who cares about grammar, just add an 's' to the name
      attParElem = doc_.createElement(attName + QString::fromLatin1("s"));
      unitElem.appendChild(attParElem);
      
      // the space after the semi-colon is enforced when the attribute is set for the unit
      QStringList atts = QStringList::split(QString::fromLatin1("; "), attValue);
      QStringList::ConstIterator it;
      for(it = atts.begin(); it != atts.end(); ++it) {
        attElem = doc_.createElement(attName);
        // never going to be BCAttribute::Bool, so don't bother to check
        attElem.appendChild(doc_.createTextNode(*it));
        attParElem.appendChild(attElem);
      }
    } else {
      attElem = doc_.createElement(attName);
      unitElem.appendChild(attElem);
      if(attIt.current()->type() != BCAttribute::Bool) {
        attElem.appendChild(doc_.createTextNode(attValue));
      }
    }
  } // end attribute loop
  
  parent_.appendChild(unitElem);
}
