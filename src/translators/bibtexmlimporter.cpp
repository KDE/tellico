/***************************************************************************
    Copyright (C) 2003-2018 Robby Stephenson <robby@periapsis.org>
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

#include "bibtexmlimporter.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

using Tellico::Import::BibtexmlImporter;

BibtexmlImporter::BibtexmlImporter(const QUrl& url) : Import::XSLTImporter(url) {
  init();
}

BibtexmlImporter::BibtexmlImporter(const QString& text) : Import::XSLTImporter(text) {
  init();
}

void BibtexmlImporter::init() {
  QString xsltFile = DataFileRegistry::self()->locate(QStringLiteral("bibtexml2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    QUrl u = QUrl::fromLocalFile(xsltFile);
    XSLTImporter::setXSLTURL(u);
  } else {
    myWarning() << "unable to find bibtexml2tellico.xsl!";
  }
}

bool BibtexmlImporter::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}
