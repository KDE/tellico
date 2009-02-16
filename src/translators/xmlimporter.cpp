/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "xmlimporter.h"
#include "../filehandler.h"
#include "../collection.h"

#include <klocale.h>

using Tellico::Import::XMLImporter;

XMLImporter::XMLImporter(const KUrl& url_) : Import::Importer(url_) {
  if(!url_.isEmpty() && url_.isValid()) {
    m_dom = FileHandler::readXMLFile(url_, true);
  }
}

XMLImporter::XMLImporter(const QString& text_) : Import::Importer(text_) {
  if(text_.isEmpty()) {
    return;
  }
  setText(text_);
}

XMLImporter::XMLImporter(const QByteArray& data_) : Import::Importer(KUrl()) {
  if(data_.isEmpty()) {
    return;
  }

  QString errorMsg;
  int errorLine, errorColumn;
  if(!m_dom.setContent(data_, true, &errorMsg, &errorLine, &errorColumn)) {
    QString str = i18n("There is an XML parsing error in line %1, column %2.", errorLine, errorColumn);
    str += QString::fromLatin1("\n");
    str += i18n("The error message from Qt is:");
    str += QString::fromLatin1("\n\t") + errorMsg;
    setStatusMessage(str);
    return;
  }
}

XMLImporter::XMLImporter(const QDomDocument& dom_) : Import::Importer(KUrl()), m_dom(dom_) {
}

void XMLImporter::setText(const QString& text_) {
  Importer::setText(text_);
  QString errorMsg;
  int errorLine, errorColumn;
  if(!m_dom.setContent(text_, true, &errorMsg, &errorLine, &errorColumn)) {
    QString str = i18n("There is an XML parsing error in line %1, column %2.", errorLine, errorColumn);
    str += QString::fromLatin1("\n");
    str += i18n("The error message from Qt is:");
    str += QString::fromLatin1("\n\t") + errorMsg;
    setStatusMessage(str);
  }
}

Tellico::Data::CollPtr XMLImporter::collection() {
  return Data::CollPtr();
}

#include "xmlimporter.moc"
