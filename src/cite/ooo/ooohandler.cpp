/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>

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

#include "ooohandler.h"
#include "interface.h"

#include <iostream>

extern "C" {
  Tellico::Cite::Handler* handler() {
    return new Tellico::Cite::OOOHandler();
  }
}

using Tellico::Cite::OOOHandler;
Tellico::Cite::Map OOOHandler::s_fieldsMap;

/*
QString OOOHandler::OUString2Q(const rtl::OUString& str) {
  const uint len = str.getLength();
  QChar* uni = new QChar[len + 1];

  const sal_Unicode* ouPtr = str.getStr();
  const sal_Unicode* ouEnd = ouPtr + len;
  QChar* qPtr = uni;

  while (ouPtr != ouEnd) {
    *(qPtr++) = *(ouPtr++);
  }

  *qPtr = 0;

  QString ret(uni, len);
  delete[] uni;

  return ret;
}

rtl::OUString OOOHandler::QString2OU(const QString& str) {
  const uint len = str.length();
  sal_Unicode* uni = new sal_Unicode[len + 1];

  const QChar* qPtr = str.unicode();
  const QChar* qEnd = qPtr + len;
  sal_Unicode* uPtr = uni;

  while (qPtr != qEnd) {
    *(uPtr++) = (*(qPtr++)).unicode();
  }

  *uPtr = 0;

  rtl::OUString ret(uni, len);
  delete[] uni;

  return ret;
}
*/

void OOOHandler::buildFieldsMap() {
//  s_fieldsMap["entry-type"] = "BibliographicType";
  s_fieldsMap["entry-type"]   = "BibiliographicType"; // typo in OpenOffice
  s_fieldsMap["key"]          = "Identifier";
  s_fieldsMap["title"]        = "Title";
  s_fieldsMap["author"]       = "Author";
  s_fieldsMap["booktitle"]    = "Booktitle";
  s_fieldsMap["address"]      = "Address";
  s_fieldsMap["chapter"]      = "Chapter";
  s_fieldsMap["edition"]      = "Edition";
  s_fieldsMap["editor"]       = "Editor";
  s_fieldsMap["organization"] = "Organizations"; // OOO has an 's'
  s_fieldsMap["publisher"]    = "Publisher";
  s_fieldsMap["pages"]        = "Pages";
  s_fieldsMap["howpublished"] = "Howpublished";
  s_fieldsMap["institution"]  = "Institution";
  s_fieldsMap["journal"]      = "Journal";
  s_fieldsMap["month"]        = "Month";
  s_fieldsMap["number"]       = "Number";
  s_fieldsMap["note"]         = "Note";
  s_fieldsMap["annote"]       = "Annote";
  s_fieldsMap["series"]       = "Series";
  s_fieldsMap["volume"]       = "Volume";
  s_fieldsMap["year"]         = "Year";
  s_fieldsMap["url"]          = "URL";
  s_fieldsMap["isbn"]         = "ISBN";
}

OOOHandler::OOOHandler() : Handler(), m_interface(0), m_state(NoConnection) {
}

Tellico::Cite::State OOOHandler::state() const {
  // possibly the write got closed underneath us
  if(m_state != NoConnection && m_interface && !m_interface->isConnected()) {
    m_state = NoConnection;
  }
  return m_state;
}

bool OOOHandler::connect() {
  if(!m_interface) {
    m_interface = new Interface();
  }

  if(!m_interface->connect(host(), port(), pipe())) {
    return false;
  }

  if(!m_interface->createDocument()) {
    m_state = NoDocument;
    return false;
  }

  m_state = NoCitation;
  return true;
}

bool OOOHandler::cite(Map& fields) {
  if(!m_interface && !connect()) {
    return false;
  }
  Cite::Map newFields = convertFields(fields);
  // the ooo interface can insert records in the database, but the citations in the
  // document ARE NOT linked to them, meaning they aren't updated by changing the database
  // the user has to manually edit each entry
  bool success = m_interface->insertCitations(newFields) && m_interface->updateBibliography();
//  bool success = m_interface->insertRecords(newFields);
  if(success) {
    m_state = Success;
//    m_interface->disconnect();
//    m_state = NoConnection;
  }
  return success;
}

Tellico::Cite::Map OOOHandler::convertFields(Cite::Map& fields) {
  if(s_fieldsMap.empty()) {
    buildFieldsMap();
  }

  Cite::Map values;
  for(Cite::Map::iterator it = s_fieldsMap.begin(); it != s_fieldsMap.end(); ++it) {
    std::string value = fields[it->first];
    if(!value.empty()) {
      values[it->second] = value;
    }
  }

  return values;
}
