/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#include "importer.h"

#include <KLocalizedString>

using Tellico::Import::Importer;

// really according to taste or computer speed
const unsigned Importer::s_stepSize = 20;

Importer::Importer() : QObject(), m_options(ImportProgress | ImportShowImageErrors) {
}

Importer::Importer(const QUrl& url) : QObject(), m_options(ImportProgress | ImportShowImageErrors) {
  m_urls << url;
}

Importer::Importer(const QList<QUrl>& urls) : QObject(), m_options(ImportProgress | ImportShowImageErrors), m_urls(urls) {
}

Importer::Importer(const QString& text) : QObject(), m_options(ImportProgress | ImportShowImageErrors), m_text(text) {
}

Tellico::Data::CollPtr Importer::currentCollection() const {
  return m_currentCollection;
}

QString Importer::progressLabel() const {
  return url().isEmpty() ?
         i18n("Loading data...") :
         i18n("Loading %1...", url().fileName());
}
