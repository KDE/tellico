/***************************************************************************
                           bookcasexmlimporter.cpp
                             -------------------
    begin                : Wed Sep 24 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bookcasexmlimporter.h"
#include "bookcasexmlexporter.h" // needed solely for BookcaseDoc::syntaxVersion
#include "../bccollectionfactory.h"
#include "../collections/bibtexcollection.h"
#include "../collections/bibtexattribute.h"
#include "../bcfilehandler.h"

#include <klocale.h>
#include <kdebug.h>

#if QT_VERSION < 0x030100
#include <qregexp.h> // needed for string replacement
#endif

/*
 * VERSION 2 added namespaces, changed to multiple elements,
 * and changed the "keywords" attribute to "keyword"
 *
 * VERSION 3 broke out the formatFlag, and changed NoComplete to AllowCompletion
 *
 * VERSION 4 added a bibtex-field name for Bibtex collections, element name was
 * changed to 'entry', attribute elements changed to 'field', and boolean fields are now "true"
 */

BookcaseXMLImporter::BookcaseXMLImporter(const KURL& url_) : TextImporter(url_), m_coll(0) {
  m_doc = BCFileHandler::readXMLFile(url_);
}

BookcaseXMLImporter::BookcaseXMLImporter(const QString& text_) : TextImporter(KURL()), m_coll(0) {
  readDomDocument(text_);
}

BCCollection* BookcaseXMLImporter::collection() {
  if(!m_coll) {
    loadDomDocument();
  }
  return m_coll;
}

void BookcaseXMLImporter::readDomDocument(const QString& text_) {
  if(text_.isEmpty()) {
    return;
  }

  // Is it XML ?
  if(text_.left(5) != QString::fromLatin1("<?xml")) {
    setStatusMessage(i18n("Bookcase is unable to load the file - %1.").arg(url().fileName()));
    return;
  }

  QString errorMsg;
  int errorLine, errorColumn;
  if(!m_doc.setContent(text_, false, &errorMsg, &errorLine, &errorColumn)) {
    QString str = i18n("Bookcase is unable to load the file - %1.").arg(url().fileName());
    str += i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
    str += QString::fromLatin1("\n");
    str += i18n("The error message from Qt is:");
    str += QString::fromLatin1("\n\t") + errorMsg;
    setStatusMessage(str);
    return;
  }
}

void BookcaseXMLImporter::loadDomDocument() {
  QDomElement root = m_doc.documentElement();
  if(root.tagName() != QString::fromLatin1("bookcase")) {
    setStatusMessage(i18n("Bookcase is unable to load the file - %1.").arg(url().fileName()));
    return;
  }

  // the syntax version attribute name changed from "version" to "syntaxVersion" in version 3
  unsigned syntaxVersion;
  if(root.hasAttribute(QString::fromLatin1("syntaxVersion"))) {
    syntaxVersion = root.attribute(QString::fromLatin1("syntaxVersion")).toInt();
  } else if (root.hasAttribute(QString::fromLatin1("version"))) {
    syntaxVersion = root.attribute(QString::fromLatin1("version")).toInt();
  } else {
    setStatusMessage(i18n("Bookcase is unable to load the file - %1.").arg(url().fileName()));
    return;
  }

  if(syntaxVersion > BookcaseXMLExporter::syntaxVersion) {
    QString str = i18n("Bookcase is unable to load the file - %1.").arg(url().fileName());
    str += QString::fromLatin1("\n");
    str += i18n("It is from a future version of Bookcase.");
    setStatusMessage(str);
    return;
  } else if(syntaxVersion < BookcaseXMLExporter::syntaxVersion) {
    QString str = i18n("Bookcase is converting the file to a more recent document format. "
                       "Information loss may occur if an older version of Bookcase is used "
                       "to read this file in the future.");
    kdDebug() << str <<  endl;
//    setStatusMessage(str);
  }

  QDomNodeList collelems = root.elementsByTagName(QString::fromLatin1("collection"));
  if(collelems.count() > 1) {
    kdWarning() << "BookcaseXMLImporter::loadDomDocument() - There is more than one collection."
                   "This isn't supported at the moment. Only the first will be loaded." << endl;
  }

  QDomElement collelem = collelems.item(0).toElement();
  QString title = collelem.attribute(QString::fromLatin1("title"));
  QString unitTitle = collelem.attribute(QString::fromLatin1("unitTitle"));

  QDomNodeList attelems;
  if(syntaxVersion < 4) {
    attelems = collelem.elementsByTagName(QString::fromLatin1("attribute"));
  } else {
    attelems = collelem.elementsByTagName(QString::fromLatin1("field"));
  }
//  kdDebug() << "BookcaseDoc::loadDomDocument() - " << attelems.count() << " attribute(s)" << endl;

  // the dilemma is when to force the new collection to have all the default attributes
  // if there are no attributes or if the first one is not the title, then add defaults
  bool addAttributes = (attelems.count() == 0);
  if(!addAttributes) {
    QString name = attelems.item(0).toElement().attribute(QString::fromLatin1("name"));
    addAttributes = (name != QString::fromLatin1("title"));
  }

  QString unitName;

  // in syntax 4, the element name was changed to "entry", always, rather than depending on
  // on the unitName of the collection. A type attribute was added to the collection element
  // to specify what type of collection it is.
  if(syntaxVersion < 4) {
    unitName = collelem.attribute(QString::fromLatin1("unit"));
    
    m_coll = BCCollectionFactory::collection(unitName, addAttributes);
  } else {
    unitName = QString::fromLatin1("entry");
    QString typeStr = collelem.attribute(QString::fromLatin1("type"));
    BCCollection::CollectionType type = static_cast<BCCollection::CollectionType>(typeStr.toInt());
    m_coll = BCCollectionFactory::collection(type, addAttributes);
  }

  if(!title.isEmpty()) {
    m_coll->setTitle(title);
  }

  for(unsigned j = 0; j < attelems.count(); ++j) {
    readAttribute(syntaxVersion, attelems.item(j).toElement());
  }

  if(m_coll->collectionType() == BCCollection::Bibtex) {
    BibtexCollection* c = dynamic_cast<BibtexCollection*>(m_coll);
    QDomNodeList macroelems = collelem.elementsByTagName(QString::fromLatin1("macro"));
//    kdDebug() << "BookcaseDoc::loadDomDocument() - found " << macroelems.count() << " macros" << endl;
    for(unsigned j = 0; c && j < macroelems.count(); ++j) {
      QDomElement elem = macroelems.item(j).toElement();
      c->addMacro(elem.attribute(QString::fromLatin1("name")), elem.text());
    }

    QDomNodeList preelems = collelem.elementsByTagName(QString::fromLatin1("bibtex-preamble"));
    if(preelems.count() > 0) {
      QString pre = preelems.item(0).toElement().text();
      c->setPreamble(pre);
    }
  }

  QDomNodeList unitelems = collelem.elementsByTagName(unitName);
//  kdDebug() << QString("BookcaseDoc::loadDomDocument() - There are %1 %2(s) "
//                         "in the collection.").arg(unitelems.count()).arg(unitName) << endl;

//  as a special case, for old book collections with a bibtex-id field, convert to Bibtex
  if(syntaxVersion < 4 && m_coll->collectionType() == BCCollection::Book
     && m_coll->attributeByName(QString::fromLatin1("bibtex-id")) != 0) {
    BibtexCollection* c = BibtexCollection::convertBookCollection(m_coll);
    delete m_coll;
    m_coll = c;
  }

  unsigned count = unitelems.count();
  for(unsigned j = 0; j < count; ++j) {
    readUnit(syntaxVersion, unitelems.item(j));

    if(j%s_stepSize == 0) {
      emit signalFractionDone(static_cast<float>(j)/static_cast<float>(count));
    }
  } // end unit loop

  return;
}

void BookcaseXMLImporter::readAttribute(unsigned syntaxVersion, const QDomElement& attelem) {
  QString attName  = attelem.attribute(QString::fromLatin1("name"), QString::fromLatin1("unknown"));
  QString attTitle = attelem.attribute(QString::fromLatin1("title"), i18n("Unknown"));

  QString attTypeStr = attelem.attribute(QString::fromLatin1("type"), QString::number(BCAttribute::Line));
  BCAttribute::AttributeType attType = static_cast<BCAttribute::AttributeType>(attTypeStr.toInt());

  // if it's a bibtex collection, BibtexAttributes will be used
  BCAttribute* att;
  if(m_coll->collectionType() == BCCollection::Bibtex) {
    if(attType == BCAttribute::Choice) {
      QString attAllowed = attelem.attribute(QString::fromLatin1("allowed"));
      att = new BibtexAttribute(attName, attTitle, QStringList::split(QString::fromLatin1(";"), attAllowed));
    } else {
      att = new BibtexAttribute(attName, attTitle, attType);
    }
  } else {
    if(attType == BCAttribute::Choice) {
      QString attAllowed = attelem.attribute(QString::fromLatin1("allowed"));
      att = new BCAttribute(attName, attTitle, QStringList::split(QString::fromLatin1(";"), attAllowed));
    } else {
      att = new BCAttribute(attName, attTitle, attType);
    }
  }

  if(attelem.hasAttribute(QString::fromLatin1("category"))) {
    // at one point, the categories had keyboard accels
    QString cat = attelem.attribute(QString::fromLatin1("category"));
    if(cat.find('&') > -1) {
      // Qt 3.0.x doesn't have QString::replace(QChar, ...)
#if QT_VERSION >= 0x030100
      cat.replace('&', QString::null);
#else
      cat.replace(QRegExp(QString::fromLatin1("&")), QString::null);
#endif
    }
    att->setCategory(cat);
  }

  if(attelem.hasAttribute(QString::fromLatin1("flags"))) {
    int flags = attelem.attribute(QString::fromLatin1("flags")).toInt();
    // I also changed the enum values for syntax 3, but the only custom attribute
    // would have been bibtex-id
    if(syntaxVersion < 3 && att->name() == QString::fromLatin1("bibtex-id")) {
      flags = 0;
    }

    // in syntax version 4, added a flag to disallow deleting attributes
    // if it's a version before that and is the title, then add the flag
    if(syntaxVersion < 4 && att->name() == QString::fromLatin1("title")) {
      flags |= BCAttribute::NoDelete;
    }
    att->setFlags(flags);
  }

  QString attFormatStr = attelem.attribute(QString::fromLatin1("format"),
                                           QString::number(BCAttribute::FormatPlain));
  BCAttribute::FormatFlag format = static_cast<BCAttribute::FormatFlag>(attFormatStr.toInt());
  att->setFormatFlag(format);

  if(attelem.hasAttribute(QString::fromLatin1("description"))) {
    att->setDescription(attelem.attribute(QString::fromLatin1("description")));
  }

  if(att->isBibtexAttribute()) {
    BibtexAttribute* batt = dynamic_cast<BibtexAttribute*>(att);
    if(attelem.hasAttribute(QString::fromLatin1("bibtex-field"))) {
      batt->setBibtexFieldName(attelem.attribute(QString::fromLatin1("bibtex-field")));
    }
  }

  m_coll->addAttribute(att);
//  kdDebug() << QString("  Added attribute: %1, %2").arg(att->name()).arg(att->title()) << endl;
}

void BookcaseXMLImporter::readUnit(unsigned syntaxVersion, const QDomNode& unitNode) {
  BCUnit* unit = new BCUnit(m_coll);
  // ierate over all attribute value children
  for(QDomNode attNode = unitNode.firstChild(); !attNode.isNull(); attNode = attNode.nextSibling()) {
    // BCUnit::setAttribute checks to see if an attribute of 'name' is allowed
    // in version 3 and prior, checkbox attributes had no text(), set it to "true" now
    if(syntaxVersion < 4 && attNode.toElement().text().isEmpty()) {
      // "true" means checked
      unit->setAttribute(attNode.toElement().tagName(), QString::fromLatin1("true"));
    } else {
      QString attName = attNode.toElement().tagName();
      // if the first child of the attNode is a text node, just set the attribute text
      // otherwise, recurse over the attNode's children
      // this is the case for <authors><author>..</author></authors>
      if(attNode.firstChild().nodeType() == QDomNode::TextNode) {
        // in version 2, "keywords" changed to "keyword"
        if(syntaxVersion < 2 && attName == QString::fromLatin1("keywords")) {
          attName = QString::fromLatin1("keyword");
        }
        unit->setAttribute(attName, attNode.toElement().text());
      } else { // if not, then it has children, iterate through them
        // the attName has the final 's', so remove it
        attName.truncate(attName.length() - 1);
        QString attValue;
        QDomNode attChildNode = attNode.firstChild();
        // is it a 2-column table
        bool table2 = (m_coll->attributeByName(attName)->type() == BCAttribute::Table2);
        // concatenate values
        for( ; !attChildNode.isNull(); attChildNode = attChildNode.nextSibling()) {
          if(table2) {
            attValue += attChildNode.firstChild().toElement().text();
            attValue += QString::fromLatin1("::");
            attValue += attChildNode.lastChild().toElement().text();
          } else {
            attValue += attChildNode.toElement().text();
          }
          attValue += QString::fromLatin1("; ");
        }
        // remove the last semi-colon and space
        attValue.truncate(attValue.length() - 2);
        unit->setAttribute(attName, attValue);
      }
    }
  } // end attribute value loop

  // no need to call slotAddUnit, it just sends a signal and changes modified flag
  m_coll->addUnit(unit);
}
