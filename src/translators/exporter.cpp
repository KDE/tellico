/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "exporter.h"
#include "../collection.h"
#include "../tellico_debug.h"

using Tellico::Export::Exporter;

Exporter::Exporter(Tellico::Data::CollPtr coll_, const QUrl& baseUrl_)
  : QObject(),
    m_options(Export::ExportUTF8),
    m_coll(coll_),
    m_baseUrl(baseUrl_) {
}

Exporter::~Exporter() {
}

Tellico::Data::CollPtr Exporter::collection() const {
  if(m_coll) {
    return m_coll;
  }
  if(!m_entries.isEmpty()) {
    return m_entries.first()->collection();
  }
  myWarning() << "no collection set";
  return Data::CollPtr();
}

const Tellico::Data::FieldList& Exporter::fields() const {
  return m_fields.isEmpty() ? collection()->fields() : m_fields;
}
