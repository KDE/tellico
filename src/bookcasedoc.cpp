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

#include <libxml/HTMLtree.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

const int FORMAT_VERSION = 1;
const int OPEN_SIGNAL_STEP_SIZE = 10;
const int SAVE_SIGNAL_STEP_SIZE = 10;

extern int xmlLoadExtDtdDefaultValue;

/* some functions to pass to the XSLT libs */
int writeToQString(void * context, const char * buffer, int len) {
  QString *t = (QString*)context;
  *t += QString::fromUtf8(buffer, len);
  return len;
}

void closeQString(void * context) {
  QString *t = (QString*)context;
  *t += QString::fromLatin1("\n");
}


BookcaseDoc::BookcaseDoc(QWidget* parent_, const char* name_/*=0*/)
 : QObject(parent_, name_), m_isModified(false) {
  m_collList.setAutoDelete(true);

//  newDocument();
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
//  kdDebug() << "BookcaseDoc::newDocument()" << endl;
  deleteContents();

  // a new document always has an empty book collection
  // the 0 is the collection number
  BCCollection* coll = BCCollection::Books(0);

  // can't call slotAddCollection() because setModified(true) gets called
  m_collList.append(coll);
  emit signalCollectionAdded(coll);

  setModified(false);
  m_url.setFileName(i18n("Untitled"));

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
  //emit signalNewDoc();
  deleteContents();
  setURL(url_);

  QDomNodeList collelems = root.elementsByTagName("collection");
  if(collelems.count() > 1) {
    kdWarning() << "BookcaseDoc::openDocument() - There is more than one collection." << endl;
  }

// for now, don't support more than one collection
//  for(unsigned i = 0; i < collelems.count(); ++i) {
  for(unsigned i = 0; i < 1; ++i) {
    QDomElement collelem = collelems.item(i).toElement();
    QString title = collelem.attribute("title");
    QString unit = collelem.attribute("unit");
    QString unitTitle = collelem.attribute("unitTitle");
    BCCollection* coll;
    if(unit == "book") {
      coll = BCCollection::Books(collectionCount());
#if 0
    } else if(unit == "cd") {
      coll = BCCollection::CDs(collectionCount());
    } else if(unit == "video") {
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
  QDomDocument dom = exportXML();

  if(KIO::NetAccess::exists(url_)) {
    KURL backup(url_);
    backup.setPath(backup.path() + QString::fromLatin1("~"));
    KIO::NetAccess::del(backup);
    KIO::NetAccess::copy(url_, backup);
  }

  bool success;
  if(url_.isLocalFile()) {
    QFile f(url_.path());
    success = writeDocument(f, dom.toCString());
    f.close();
  } else {
    KTempFile tempfile;
    QFile f(tempfile.name());
    success = writeDocument(f, dom.toCString());
    f.close();
    KIO::NetAccess::upload(tempfile.name(), url_);
    tempfile.unlink();
  }

  // now we're really done
//  emit signalFractionDone(1.0);

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
  BCCollection* coll = 0;
  QListIterator<BCCollection> it(m_collList);
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

const QList<BCCollection>& BookcaseDoc::collectionList() const{
  return m_collList;
}

QDomDocument BookcaseDoc::exportXML() {
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
        attElem.setAttribute("description", attIt.current()->description());
      } // end attribute loop
     } // end attribute save

    QDomElement attElem;
    QDomElement unitElem;
    QListIterator<BCAttribute> attIt(coll->attributeList());
    QListIterator<BCUnit> unitIt(coll->unitList());
    // the j counter is for progress
    for(int j = 0; unitIt.current(); ++unitIt, ++j) {
      unitElem = doc.createElement(coll->unitName());
      collElem.appendChild(unitElem);
      for(attIt.toFirst(); attIt.current(); ++attIt) {
        QString attValue = unitIt.current()->attribute(attIt.current()->name());
        if(!attValue.isEmpty()) {
          attElem = doc.createElement(attIt.current()->name());
          unitElem.appendChild(attElem);
          if(attIt.current()->type() != BCAttribute::Bool) {
            attElem.appendChild(doc.createTextNode(attValue));
          }
        }
      } // end attribute loop
      
//      if(j%SAVE_SIGNAL_STEP_SIZE == 0) {
//        // allocate equal time to each collection in document, leave 20% for later file write
//        // i is number of collection
//        // j is number of unit
//        float f = i+static_cast<float>(j)/static_cast<float>(coll->unitCount());
//        f *= 0.8/static_cast<float>(collectionCount());
//        emit signalFractionDone(f);
//      }
    } // end unit loop
  } // end collection loop

  return doc;
}

QString BookcaseDoc::exportHTML(const QString& xsltFilename_, bool format_/*=false*/) {
  QString html;
  QDomDocument dom;
  
  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue = 1;

  dom = exportXML();
  Bookcase* app = static_cast<Bookcase*>(parent());
  dom = app->collectionViewTree();
  if(format_) {
    // TODO: fix this if multiple collections become supported
    QListIterator<BCAttribute> attIt(collectionById(0)->attributeList());
    int flags;
    QDomNodeList nodes;
    QDomNode textNode;
    QString text;
    for( ; attIt.current(); ++attIt) {
      flags = attIt.current()->flags();
      nodes = dom.elementsByTagName(attIt.current()->name());
//      kdDebug() << nodes.count() << " nodes for " << attIt.current()->name() << endl;
      for(unsigned i = 0; i < nodes.count(); ++i) {
        textNode = nodes.item(i).firstChild();
        if(textNode.isText() && !textNode.nodeValue().isEmpty()) {
          text = textNode.nodeValue();
          if(flags & BCAttribute::FormatTitle) {
            text = BCAttribute::formatTitle(text);
          } else if(flags & BCAttribute::FormatName) {
            text = BCAttribute::formatName(text, (flags & BCAttribute::AllowMultiple));
          } else if(flags & BCAttribute::FormatDate) {
            text = BCAttribute::formatDate(text);
          }
          textNode.setNodeValue(text);
          // nodes.item(i).replaceChild(dom.createTextNode(text), textNode);
        }
      } // end node loop
    } // end attribute loop
  }
  xmlDocPtr doc = xmlParseDoc((xmlChar *)dom.toCString().data());

  xsltStylesheetPtr stylesheet = xsltParseStylesheetFile((const xmlChar *)xsltFilename_.latin1());
  if(!stylesheet) {
    return html;
  }

  const char *params[1];
  params[0] = NULL;
  xmlDocPtr out = xsltApplyStylesheet(stylesheet, doc, params);

  xmlOutputBufferPtr outp = xmlOutputBufferCreateIO(writeToQString,
                              (xmlOutputCloseCallback)closeQString, &html, 0);
  outp->written = 0;
  xsltSaveResultTo(outp, out, stylesheet);
  xmlOutputBufferFlush(outp);

  xsltFreeStylesheet(stylesheet);
  xmlFreeDoc(out);
  xmlFreeDoc(doc);
  xsltCleanupGlobals();
  xmlCleanupParser();

  return html;
}

void BookcaseDoc::slotAddCollection(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

  m_collList.append(coll_);
  emit signalCollectionAdded(coll_);

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
