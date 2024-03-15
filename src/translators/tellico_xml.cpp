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
#include "../tellico_debug.h"

#include <libxml/parserInternals.h> // needed for IS_LETTER
#include <libxml/parser.h> // has to be before valid.h
#include <libxml/valid.h>

#include <QRegularExpression>

const QString Tellico::XML::nsXSL = QStringLiteral("http://www.w3.org/1999/XSL/Transform");
const QString Tellico::XML::nsBibtexml = QStringLiteral("http://bibtexml.sf.net/");
const QString Tellico::XML::dtdBibtexml = QStringLiteral("bibtexml.dtd");

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
 * VERSION 11 remove ReadOnly and Dependent fields, and added appropriate FieldFlags. An ID field was added by default.
 *
 * VERSION 12 added new filter rules: before and after, less than and greater than. But only use v12 when needed
 */
const uint Tellico::XML::syntaxVersion = 12;
const QString Tellico::XML::nsTellico = QStringLiteral("http://periapsis.org/tellico/");

const QString Tellico::XML::nsBookcase = QStringLiteral("http://periapsis.org/bookcase/");
const QString Tellico::XML::nsDublinCore = QStringLiteral("http://purl.org/dc/elements/1.1/");
const QString Tellico::XML::nsZing = QStringLiteral("http://www.loc.gov/zing/srw/");
const QString Tellico::XML::nsZingDiag = QStringLiteral("http://www.loc.gov/zing/srw/diagnostic/");
const QString Tellico::XML::nsAtom = QStringLiteral("http://www.w3.org/2005/Atom");
const QString Tellico::XML::nsOpenSearch = QStringLiteral("http://a9.com/-/spec/opensearch/1.1/");
const QString Tellico::XML::nsOpenPackageFormat = QStringLiteral("http://www.idpf.org/2007/opf");

QString Tellico::XML::pubTellico(int version) {
 return QStringLiteral("-//Robby Stephenson/DTD Tellico V%1.0//EN").arg(version);
}

QString Tellico::XML::dtdTellico(int version) {
  return QStringLiteral("http://periapsis.org/tellico/dtd/v%1/tellico.dtd").arg(version);
}

bool Tellico::XML::validXMLElementName(const QString& name_) {
  return xmlValidateNCName((xmlChar *)name_.toUtf8().data(), 0) == 0;
}

QString Tellico::XML::elementName(const QString& name_) {
  static const QRegularExpression whitespace(QStringLiteral("\\s+"));
  QString name = name_;
  // change white space to dashes
  name.replace(whitespace, QStringLiteral("-"));
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

QByteArray Tellico::XML::recoverFromBadXMLName(const QByteArray& data_) {
  // this is going to be ugly (Bug 418067)
  // Do a rough parse of the data, grab the field names, determine which ones are invalid
  // then search/replace to recover. Let's assume the XML format is as written directly from Tellico
  // so don't worry about attribute order within the field elements
  const int fieldsEnd = data_.indexOf("</fields>");
  if(fieldsEnd == -1) {
    return data_;
  }

  QByteArray newData = data_;

  typedef QPair<QByteArray, QByteArray> ByteArrayPair;
  // keep a list of pairs to replace
  QList<ByteArrayPair> badNames;
  // an expensive conversion, but have to convert to a string
  const QString fieldsSection = QString::fromUtf8(data_.left(fieldsEnd));
  QString newFieldsSection = fieldsSection;
  QRegularExpression fieldNameRX(QStringLiteral("<field .*?name=\"(.+?)\".*?>"));
  QRegularExpressionMatchIterator i = fieldNameRX.globalMatch(fieldsSection);
  while(i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    const QString fieldName = match.captured(1);
    if(!validXMLElementName(fieldName)) {
      const QString newName = elementName(fieldName);
      if(newName.isEmpty()) {
        return data_;
      }
      myDebug() << "Bad name is" << fieldName << "; Good name is" << newName;
      badNames += qMakePair(fieldName.toUtf8().prepend('<').append('>'),
                            newName.toUtf8().prepend('<').append('>'));
      badNames += qMakePair(fieldName.toUtf8().prepend("</").append('>'),
                            newName.toUtf8().prepend("</").append('>'));
      // also have to check for plurals
      badNames += qMakePair(fieldName.toUtf8().prepend('<').append("s>"),
                            newName.toUtf8().prepend('<').append("s>"));
      badNames += qMakePair(fieldName.toUtf8().prepend("</").append("s>"),
                            newName.toUtf8().prepend("</").append("s>"));
      // the bad name might be in the description attribute which is fine, leave it alone
      newFieldsSection.replace(QStringLiteral("name=\"") + fieldName,
                               QStringLiteral("name=\"") + newName);
    }
  }

  // if there are no fields to replace, we're done
  if(badNames.isEmpty()) {
    return data_;
  }

  // swap out the new fields header
  newData.replace(0, fieldsEnd, newFieldsSection.toUtf8());

  foreach(const ByteArrayPair& ii, badNames) {
//    myDebug() << "Replacing" << ii.first << "with" << ii.second;
    newData.replace(ii.first, ii.second);
  }
  return newData;
}

QByteArray Tellico::XML::removeInvalidXml(const QByteArray& data_) {
  const uint len = data_.length();
  QByteArray result;
  result.reserve(len);
  for(uint i = 0; i < len; ++i) {
    auto c = data_.at(i);
    // for now, stick with anything below #x20 except for #x9 | #xA | #xD
    if(c >= 0x20 || c == 0x09 || c == 0x0A || c == 0x0D) {
      result.append(c);
    }
  }
  result.squeeze();
  return result;
}
