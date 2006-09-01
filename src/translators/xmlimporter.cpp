/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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

#include <klocale.h>

using Tellico::Import::XMLImporter;

XMLImporter::XMLImporter(const KURL& url_) : Import::Importer(url_) {
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

XMLImporter::XMLImporter(const QByteArray& data_) : Import::Importer(KURL()) {
  if(data_.isEmpty()) {
    return;
  }

  QString errorMsg;
  int errorLine, errorColumn;
  if(!m_dom.setContent(data_, true, &errorMsg, &errorLine, &errorColumn)) {
    QString str = i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
    str += QString::fromLatin1("\n");
    str += i18n("The error message from Qt is:");
    str += QString::fromLatin1("\n\t") + errorMsg;
    setStatusMessage(str);
    return;
  }
}

void XMLImporter::setText(const QString& text_) {
  Importer::setText(text_);
  QString errorMsg;
  int errorLine, errorColumn;
  if(!m_dom.setContent(text_, true, &errorMsg, &errorLine, &errorColumn)) {
    QString str = i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
    str += QString::fromLatin1("\n");
    str += i18n("The error message from Qt is:");
    str += QString::fromLatin1("\n\t") + errorMsg;
    setStatusMessage(str);
  }
}

#include "xmlimporter.moc"
