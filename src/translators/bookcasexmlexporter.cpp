/***************************************************************************
                           bookcasexmlexporter.cpp
                             -------------------
    begin                : Wed Sep 10 2003
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

#include "bookcasexmlexporter.h"
#include "../bccollection.h"
#include "../collections/bibtexcollection.h"
#include "../collections/bibtexattribute.h"

#include <klocale.h>
#include <kdebug.h>

#include <qdom.h>
#include <qtextcodec.h>

static const char* BOOKCASE_NAMESPACE = "http://periapsis.org/bookcase/";
static const char* BOOKCASE_DTD = "bookcase.dtd";

/*
 * VERSION 2 added namespaces, changed to multiple elements,
 * and changed the "keywords" attribute to "keyword"
 *
 * VERSION 3 broke out the formatFlag, and changed NoComplete to AllowCompletion
 *
 * VERSION 4 added a bibtex-field name forBibtex collections, element name was
 * changed to 'entry', attribute elements changed to 'field', and boolean fields are now "true"
 */
const unsigned BookcaseXMLExporter::syntaxVersion = 4;

QString BookcaseXMLExporter::formatString() const {
  return i18n("Bookcase");
}

QString BookcaseXMLExporter::fileFilter() const {
  return i18n("*|All files");
}

QString BookcaseXMLExporter::text(bool format_, bool encodeUTF8_) {
  QDomDocument dom = exportXML(format_, encodeUTF8_);
  return dom.toString();
}

QDomDocument BookcaseXMLExporter::exportXML(bool format_, bool encodeUTF8_) const {
  QDomImplementation impl;
  QDomDocumentType doctype = impl.createDocumentType(QString::fromLatin1("bookcase"),
                                                     QString::null,
                                                     QString::fromLatin1(BOOKCASE_DTD));
  //default namespace
  QString ns = QString::fromLatin1(BOOKCASE_NAMESPACE);

  QDomDocument dom = impl.createDocument(ns, QString::fromLatin1("bookcase"), doctype);

  // root bookcase element
  QDomElement bcelem = dom.documentElement();

  QString encodeStr = QString::fromLatin1("version=\"1.0\" encoding=\"");
  if(encodeUTF8_) {
    encodeStr += QString::fromLatin1("UTF-8");
  } else {
    encodeStr += QString::fromLatin1(QTextCodec::codecForLocale()->mimeName());
  }
  encodeStr += QString::fromLatin1("\"");

  // createDocument creates a root node, insert the processing instruction before it
  dom.insertBefore(dom.createProcessingInstruction(QString::fromLatin1("xml"), encodeStr), bcelem);

  bcelem.setAttribute(QString::fromLatin1("syntaxVersion"), BookcaseXMLExporter::syntaxVersion);

  exportCollectionXML(dom, bcelem, format_);

  return dom;
}

void BookcaseXMLExporter::exportCollectionXML(QDomDocument& dom_, QDomElement& parent_, bool format_) const {
  QDomElement collElem = dom_.createElement(QString::fromLatin1("collection"));

  collElem.setAttribute(QString::fromLatin1("type"),      collection()->collectionType());
  collElem.setAttribute(QString::fromLatin1("title"),     collection()->title());
  collElem.setAttribute(QString::fromLatin1("unitTitle"), collection()->unitTitle());

  QDomElement attsElem = dom_.createElement(QString::fromLatin1("fields"));
  collElem.appendChild(attsElem);
  BCAttributeListIterator attIt(collection()->attributeList());
  for( ; attIt.current(); ++attIt) {
    exportAttributeXML(dom_, attsElem, attIt.current());
  }

  if(collection()->collectionType() == BCCollection::Bibtex) {
    const BibtexCollection* c = dynamic_cast<const BibtexCollection*>(collection());
    if(c) {
      if(!c->preamble().isEmpty()) {
        QDomElement preElem = dom_.createElement(QString::fromLatin1("bibtex-preamble"));
        preElem.appendChild(dom_.createTextNode(c->preamble()));
        collElem.appendChild(preElem);
      }

      QDomElement macrosElem = dom_.createElement(QString::fromLatin1("macros"));
      QMap<QString, QString>::ConstIterator macroIt;
      for(macroIt = c->macroList().begin(); macroIt != c->macroList().end(); ++macroIt) {
        if(!macroIt.data().isEmpty()) {
          QDomElement macroElem = dom_.createElement(QString::fromLatin1("macro"));
          macroElem.setAttribute(QString::fromLatin1("name"), macroIt.key());
          macroElem.appendChild(dom_.createTextNode(macroIt.data()));
          macrosElem.appendChild(macroElem);
        }
      }
      if(macrosElem.childNodes().count() > 0) {
        collElem.appendChild(macrosElem);
      }
    }
  }

  BCUnitListIterator unitIt(unitList());
  for( ; unitIt.current(); ++unitIt) {
    exportUnitXML(dom_, collElem, unitIt.current(), format_);
  }

  parent_.appendChild(collElem);
}

void BookcaseXMLExporter::exportAttributeXML(QDomDocument& dom_, QDomElement& parent_, BCAttribute* att_) const {
  QDomElement attElem = dom_.createElement(QString::fromLatin1("field"));

  attElem.setAttribute(QString::fromLatin1("name"),          att_->name());
  attElem.setAttribute(QString::fromLatin1("title"),         att_->title());
  attElem.setAttribute(QString::fromLatin1("category"),      att_->category());
  attElem.setAttribute(QString::fromLatin1("type"),          att_->type());
  attElem.setAttribute(QString::fromLatin1("flags"),         att_->flags());
  attElem.setAttribute(QString::fromLatin1("format"),        att_->formatFlag());

  if(att_->type() == BCAttribute::Choice) {
    attElem.setAttribute(QString::fromLatin1("allowed"),     att_->allowed().join(QString::fromLatin1(";")));
  }

  if(att_->isBibtexAttribute()) {
    BibtexAttribute* bAtt = dynamic_cast<BibtexAttribute*>(att_);
    if(bAtt) {
      attElem.setAttribute(QString::fromLatin1("bibtex-field"), bAtt->bibtexFieldName());
    }
  }

  // only save description if it's not equal to title, which is the default
  // title is never empty, so this indirectly checks for empty descriptions
  if(att_->description() != att_->title()) {
    attElem.setAttribute(QString::fromLatin1("description"), att_->description());
  }

  parent_.appendChild(attElem);
}

void BookcaseXMLExporter::exportUnitXML(QDomDocument& dom_, QDomElement& parent_, BCUnit* unit_, bool format_) const {
  QDomElement unitElem = dom_.createElement(QString::fromLatin1("entry"));

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
      attParElem = dom_.createElement(attName + QString::fromLatin1("s"));
      unitElem.appendChild(attParElem);

      // the space after the semi-colon is enforced when the attribute is set for the unit
      QStringList atts = QStringList::split(QString::fromLatin1("; "), attValue, true);
      for(QStringList::ConstIterator it = atts.begin(); it != atts.end(); ++it) {
        attElem = dom_.createElement(attName);
        // special case for 2-column tables
        if(attIt.current()->type() == BCAttribute::Table2) {
          QDomElement elem1, elem2;
          elem1 = dom_.createElement(QString::fromLatin1("column"));
          elem2 = dom_.createElement(QString::fromLatin1("column"));
          elem1.appendChild(dom_.createTextNode((*it).section(QString::fromLatin1("::"), 0, 0)));
          elem2.appendChild(dom_.createTextNode((*it).section(QString::fromLatin1("::"), 1)));
          attElem.appendChild(elem1);
          attElem.appendChild(elem2);
        } else {
          attElem.appendChild(dom_.createTextNode(*it));
        }
        attParElem.appendChild(attElem);
      }
    } else {
      attElem = dom_.createElement(attName);
      unitElem.appendChild(attElem);
      attElem.appendChild(dom_.createTextNode(attValue));
    }
  } // end attribute loop

  parent_.appendChild(unitElem);
}
