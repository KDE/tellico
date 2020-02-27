/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#include "tellicoxmlhandler.h"
#include "../collection.h"
#include "../tellico_debug.h"

using Tellico::Import::TellicoXMLHandler;

TellicoXMLHandler::TellicoXMLHandler() : QXmlDefaultHandler(), m_data(new SAX::StateData) {
  m_handlers.push(new SAX::RootHandler(m_data));
}

TellicoXMLHandler::~TellicoXMLHandler() {
  delete m_data;
  m_data = nullptr;
  qDeleteAll(m_handlers);
  m_handlers.clear();
}

bool TellicoXMLHandler::startElement(const QString& nsURI_, const QString& localName_,
                                     const QString& qName_, const QXmlAttributes& atts_) {
  SAX::StateHandler* handler = m_handlers.top()->nextHandler(nsURI_, localName_, qName_);
  Q_ASSERT(handler);
  m_handlers.push(handler);
  return handler->start(nsURI_, localName_, qName_, atts_);
}

bool TellicoXMLHandler::endElement(const QString& nsURI_, const QString& localName_,
                                   const QString& qName_) {
  m_data->text = m_data->text.trimmed();
/*
  if(!m_data->text.isEmpty()) {
    myDebug() << " text: " << m_text;
  }
*/

  SAX::StateHandler* handler = m_handlers.pop();
  bool res = handler->end(nsURI_, localName_, qName_);
  // need to reset character data, too
  m_data->text.clear();
  delete handler;
  return res;
}

bool TellicoXMLHandler::characters(const QString& ch_) {
  m_data->text += ch_;
  return true;
}

QString TellicoXMLHandler::errorString() const {
  return m_errorString.isEmpty() ? m_data->error : m_errorString;
}

Tellico::Data::CollPtr TellicoXMLHandler::collection() const {
  return m_data->coll;
}

bool TellicoXMLHandler::hasImages() const {
  return m_data->hasImages;
}

void TellicoXMLHandler::setLoadImages(bool loadImages_) {
  m_data->loadImages = loadImages_;
}

void TellicoXMLHandler::setShowImageLoadErrors(bool showImageErrors_) {
  m_data->showImageLoadErrors = showImageErrors_;
}

bool TellicoXMLHandler::fatalError(const QXmlParseException& exception_) {
  m_errorString = QString::fromLatin1("Fatal parsing error: %1 in line %2, column %3")
                  .arg(exception_.message())
                  .arg(exception_.lineNumber())
                  .arg(exception_.columnNumber());
  return false;
}
