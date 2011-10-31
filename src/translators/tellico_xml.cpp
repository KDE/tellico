/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "tellico_xml.h"

#include <libxml/parserInternals.h> // needed for IS_LETTER
#include <libxml/parser.h> // has to be before valid.h
#include <libxml/valid.h>

#include <QRegExp>

const QString Tellico::XML::nsXSL = QLatin1String("http://www.w3.org/1999/XSL/Transform");
const QString Tellico::XML::nsBibtexml = QLatin1String("http://bibtexml.sf.net/");
const QString Tellico::XML::dtdBibtexml = QLatin1String("bibtexml.dtd");

/*
 * VERSION 2 added namespaces, changed to multiple elements,
 * and changed the "keywords" field to "keyword"
 *
 * VERSION 3 broke out the formatType, and changed NoComplete to AllowCompletion
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
 *
 * VERSION 9 changed music collections to always have three columns by default, with title/artist/length and
 * added file catalog collection.
 *
 * VERSION 10 added the game board collection.
 *
 * VERSION 11 remove ReadOnly and Dependent fields, and added apprioriate FieldFlags. An ID field was added by default.
 *
 * VERSION 12 added new filter rules: before and after, less than and greater than. But only use v12 when needed
 */
const uint Tellico::XML::syntaxVersion = 12;
const QString Tellico::XML::nsTellico = QLatin1String("http://periapsis.org/tellico/");

const QString Tellico::XML::nsBookcase = QLatin1String("http://periapsis.org/bookcase/");
const QString Tellico::XML::nsDublinCore = QLatin1String("http://purl.org/dc/elements/1.1/");
const QString Tellico::XML::nsZing = QLatin1String("http://www.loc.gov/zing/srw/");
const QString Tellico::XML::nsZingDiag = QLatin1String("http://www.loc.gov/zing/srw/diagnostic/");

QString Tellico::XML::pubTellico(int version) {
 return QString::fromLatin1("-//Robby Stephenson/DTD Tellico V%1.0//EN").arg(version);
}

QString Tellico::XML::dtdTellico(int version) {
  return QString::fromLatin1("http://periapsis.org/tellico/dtd/v%1/tellico.dtd").arg(version);
}

bool Tellico::XML::validXMLElementName(const QString& name_) {
  return xmlValidateNameValue((xmlChar *)name_.toUtf8().data());
}

QString Tellico::XML::elementName(const QString& name_) {
  QString name = name_;
  // change white space to dashes
  name.replace(QRegExp(QLatin1String("\\s+")), QLatin1String("-"));
  // first cut, if it passes, we're done
  if(XML::validXMLElementName(name)) {
    return name;
  }

  // next check first characters IS_DIGIT is defined in libxml/vali.d
  for(int i = 0; i < name.length() && (!IS_LETTER(name[i].unicode()) || name[i] == QLatin1Char('_')); ++i) {
    name = name.mid(1);
  }
  if(name.isEmpty() || XML::validXMLElementName(name)) {
    return name; // empty names are handled later
  }

  // now brute-force it, one character at a time
  int i = 0;
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
