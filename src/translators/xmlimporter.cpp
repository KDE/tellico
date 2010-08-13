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

#include "xmlimporter.h"
#include "../core/filehandler.h"
#include "../collection.h"

#include <klocale.h>

using Tellico::Import::XMLImporter;

XMLImporter::XMLImporter(const KUrl& url_) : Import::Importer(url_) {
  if(!url_.isEmpty() && url_.isValid()) {
    m_dom = FileHandler::readXMLDocument(url_, true);
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
    str += QLatin1String("\n");
    str += i18n("The error message from Qt is:");
    str += QLatin1String("\n\t") + errorMsg;
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
    str += QLatin1String("\n");
    str += i18n("The error message from Qt is:");
    str += QLatin1String("\n\t") + errorMsg;
    setStatusMessage(str);
  }
}

Tellico::Data::CollPtr XMLImporter::collection() {
  return Data::CollPtr();
}

#include "xmlimporter.moc"
