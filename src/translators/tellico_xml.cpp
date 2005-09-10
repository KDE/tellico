/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "tellico_xml.h"

#include <libxml/parserInternals.h> // needed for IS_LETTER
#include <libxml/parser.h> // has to be before valid.h
#include <libxml/valid.h>

#include <qregexp.h>

const QString Tellico::XML::nsXSL = QString::fromLatin1("http://www.w3.org/1999/XSL/Transform");
const QString Tellico::XML::nsBibtexml = QString::fromLatin1("http://bibtexml.sf.net/");
const QString Tellico::XML::dtdBibtexml = QString::fromLatin1("bibtexml.dtd");

/*
 * VERSION 2 added namespaces, changed to multiple elements,
 * and changed the "keywords" field to "keyword"
 *
 * VERSION 3 broke out the formatFlag, and changed NoComplete to AllowCompletion
 *
 * VERSION 4 added a bibtex-field name for Bibtex collections, element name was
 * changed to 'entry', field elements changed to 'field', and boolean fields are now "true"
 *
 * VERSION 5 moved the bibtex-field and any other extended field property to property elements
 * inside the field element, and added the image element.
 *
 * VERSION 6 added id, i18n attributes, and year, month, day elements in date fields with a calendar name
 * attribute.
 *
 * VERSION 7 changed the application name to Tellico, renamed unitTitle to entryTitle, and made the id permanent.
 *
 * VERSION 8 added loans and saved filters.
 */
const uint Tellico::XML::syntaxVersion = 8;
const QString Tellico::XML::nsTellico = QString::fromLatin1("http://periapsis.org/tellico/");
const QString Tellico::XML::pubTellico = QString::fromLatin1("-//Robby Stephenson/DTD Tellico V%1.0//EN").arg(Tellico::XML::syntaxVersion);
const QString Tellico::XML::dtdTellico = QString::fromLatin1("http://periapsis.org/tellico/dtd/v%1/tellico.dtd").arg(Tellico::XML::syntaxVersion);

const QString Tellico::XML::nsBookcase = QString::fromLatin1("http://periapsis.org/bookcase/");

bool Tellico::XML::validXMLElementName(const QString& name_) {
  return xmlValidateNameValue((xmlChar *)name_.utf8().data());
}

QString Tellico::XML::elementName(const QString& name_) {
  QString name = name_;
  // change white space to dashes
  name.replace(QRegExp(QString::fromLatin1("\\s+")), QString::fromLatin1("-"));
  // first cut, if it passes, we're done
  if(XML::validXMLElementName(name)) {
    return name;
  }

  // next check first characters IS_DIGIT is defined in libxml/vali.d
  for(uint i = 0; i < name.length() && (!IS_LETTER(name[i].unicode()) || name[i] == '_'); ++i) {
    name = name.mid(1);
  }
  if(name.isEmpty() || XML::validXMLElementName(name)) {
    return name; // empty names are handled later
  }

  // now brute-force it, one character at a time
  uint i = 0;
  while(i < name.length()) {
    if(!XML::validXMLElementName(name.left(i+1))) {
      name.remove(i, 1); // remember it's zero-indexed
    } else {
      // character is ok, increment i
      ++i;
    }
  }
  return name;
}
