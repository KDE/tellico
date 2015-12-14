/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "griffithimporter.h"
#include "xslthandler.h"
#include "../collections/videocollection.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <QDir>
#include <QFile>

using Tellico::Import::GriffithImporter;

GriffithImporter::GriffithImporter(const QUrl& url_) : XSLTImporter(url_) {
  QString xsltFile = DataFileRegistry::self()->locate(QLatin1String("griffith2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    QUrl u = QUrl::fromLocalFile(xsltFile);
    setXSLTURL(u);
  } else {
    myWarning() << "unable to find griffith2tellico.xsl!";
  }
}


GriffithImporter::~GriffithImporter() {
}

void GriffithImporter::beginXSLTHandler(XSLTHandler* handler_) {
  Q_ASSERT(handler_);
  handler_->addStringParam("imgdir", QFile::encodeName(QDir::homePath() + QLatin1String("/.griffith/")));
}

bool GriffithImporter::canImport(int type) const {
  return type == Data::Collection::Video;
}
