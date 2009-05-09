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

#include "dataimporter.h"

using Tellico::Import::DataImporter;

DataImporter::DataImporter(const KUrl& url) : Importer(url), m_source(URL) {
  m_fileRef = FileHandler::fileRef(url);
}

DataImporter::DataImporter(const QString& text) : Importer(text), m_data(text.toUtf8()), m_source(Text), m_fileRef(0) {
  // remove newline
  m_data.truncate(m_data.size()-1);
}

DataImporter::~DataImporter() {
  delete m_fileRef;
  m_fileRef = 0;
}

void DataImporter::setText(const QString& text) {
  Importer::setText(text);
  m_data = text.toUtf8();
  // remove newline
  m_data.truncate(m_data.size()-1);
  m_source = Text;
}

#include "dataimporter.moc"
