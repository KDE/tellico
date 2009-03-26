/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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
  m_data = 0;
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
    myDebug() << " text: " << m_text << endl;
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
  return m_data->error;
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
