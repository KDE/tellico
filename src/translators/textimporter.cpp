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

#include "textimporter.h"
#include "../core/filehandler.h"

#include <QRegExp>

namespace {
  QString& cleanXml(QString s) {
    // remove C0 Control Characters, since we assume we're importing a file
    // with contents that will be represented in XML later...
    static const QRegExp rx(QLatin1String("[\x0000-\x00FF]"));
    return s.remove(rx);
  }
}

using Tellico::Import::TextImporter;

TextImporter::TextImporter(const KUrl& url_, bool useUTF8_)
    : Import::Importer(url_) {
  if(url_.isValid()) {
    setText(cleanXml(FileHandler::readTextFile(url_, false, useUTF8_)));
  }
}

TextImporter::TextImporter(const QString& text_) : Import::Importer(text_) {
}

#include "textimporter.moc"
