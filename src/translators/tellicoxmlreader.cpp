/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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

#include "tellicoxmlreader.h"
#include "tellico_xml.h"
#include "xmlstatehandler.h"
#include "../collection.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

using Tellico::Import::TellicoXmlReader;

TellicoXmlReader::TellicoXmlReader(const QUrl& baseUrl_) : m_data(new SAX::StateData) {
  m_data->baseUrl = baseUrl_;
  m_handlers.push(new SAX::RootHandler(m_data));
}

TellicoXmlReader::~TellicoXmlReader() {
  delete m_data;
  m_data = nullptr;
  qDeleteAll(m_handlers);
  m_handlers.clear();
}

bool TellicoXmlReader::readNext(const QByteArray& data_) {
  Q_ASSERT(!m_handlers.isEmpty());
  if(m_handlers.isEmpty()) {
    return false;
  }

  m_xml.addData(data_);
  while(!m_xml.atEnd()) {
    const auto tokenType = m_xml.readNext();
    switch(tokenType) {
      case QXmlStreamReader::StartElement:
        handleStart();
        break;
      case QXmlStreamReader::EndElement:
        handleEnd();
        break;
      case QXmlStreamReader::Characters:
        handleCharacters();
        break;
      case QXmlStreamReader::Invalid:
        if(m_xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
          myDebug() << "Error!" << m_xml.errorString();
        }
        continue; // break out of while loop
      default:
        // not handling anything else
        break;
    }
  }
  // success is no error or error being simply premature end of document
  return !m_xml.hasError() || m_xml.error() == QXmlStreamReader::PrematureEndOfDocumentError;
}

QString TellicoXmlReader::errorString() const {
  // for custom errors, include the line and column number
  return m_xml.error() == QXmlStreamReader::CustomError ?
    QStringLiteral("Fatal parsing error in line %1, column %2. %3")
                   .arg(m_xml.lineNumber())
                   .arg(m_xml.columnNumber())
                   .arg(m_xml.errorString()) :
    m_xml.errorString();
}

bool TellicoXmlReader::isNotWellFormed() const {
  return m_xml.error() == QXmlStreamReader::NotWellFormedError;
}

Tellico::Data::CollPtr TellicoXmlReader::collection() const {
  return m_data->coll;
}

bool TellicoXmlReader::hasImages() const {
  return m_data->hasImages;
}

void TellicoXmlReader::setLoadImages(bool loadImages_) {
  m_data->loadImages = loadImages_;
}

void TellicoXmlReader::setShowImageLoadErrors(bool showImageErrors_) {
  m_data->showImageLoadErrors = showImageErrors_;
}

void TellicoXmlReader::setImagePathsAsLinks(bool imagePathsAsLinks_) {
  m_data->imagePathsAsLinks = imagePathsAsLinks_;
}

void TellicoXmlReader::handleStart() {
  SAX::StateHandler* handler = m_handlers.top()->nextHandler(m_xml.namespaceUri(), m_xml.name());
  Q_ASSERT(handler);
  m_handlers.push(handler);
  if(!handler->start(m_xml.namespaceUri(), m_xml.name(), m_xml.attributes())) {
    m_xml.raiseError(m_data->error);
  }
}

void TellicoXmlReader::handleEnd() {
  m_data->text = m_data->text.trimmed();
  SAX::StateHandler* handler = m_handlers.pop();
  if(!handler->end(m_xml.namespaceUri(), m_xml.name())) {
    m_xml.raiseError(m_data->error);
  }
  // need to reset character data, too
  m_data->text.clear();
  delete handler;
}

void TellicoXmlReader::handleCharacters() {
  m_data->text.append(m_xml.text());
}
