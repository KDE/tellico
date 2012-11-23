/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "vinoxmlimporter.h"
#include "../collection.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <KStandardDirs>

using Tellico::Import::VinoXMLImporter;

VinoXMLImporter::VinoXMLImporter(const KUrl& url_) : XSLTImporter(url_) {
  QString xsltFile = KStandardDirs::locate("appdata", QLatin1String("vinoxml2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    KUrl u;
    u.setPath(xsltFile);
    XSLTImporter::setXSLTURL(u);
  } else {
    myWarning() << "unable to find vinoxml2tellico.xsl!";
  }
}

bool VinoXMLImporter::canImport(int type) const {
  return type == Data::Collection::Wine;
}

Tellico::Data::CollPtr VinoXMLImporter::collection() {
  return XSLTImporter::collection();
}

#include "vinoxmlimporter.moc"
