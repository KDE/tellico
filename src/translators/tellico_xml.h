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

#ifndef TELLICO_XML_H
#define TELLICO_XML_H

#include <QString>

namespace Tellico {
  namespace XML {
    extern const QString nsXSL;
    extern const QString nsBibtexml;
    extern const QString dtdBibtexml;

    extern const uint syntaxVersion;
    extern const QString nsTellico;

    QString pubTellico(int version = syntaxVersion);
    QString dtdTellico(int version = syntaxVersion);

    extern const QString nsBookcase;
    extern const QString nsDublinCore;
    extern const QString nsZing;
    extern const QString nsZingDiag;
    extern const QString nsAtom;
    extern const QString nsOpenSearch;
    extern const QString nsOpenPackageFormat;

    bool validXMLElementName(const QString& name);
    QString elementName(const QString& name);
    QByteArray recoverFromBadXMLName(const QByteArray& data);
    QByteArray removeInvalidXml(const QByteArray& data);
  }
}

#endif
