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

#ifndef TELLICO_XML_H
#define TELLICO_XML_H

#include <qstring.h>

namespace Tellico {
  namespace XML {
    extern const QString nsXSL;
    extern const QString nsBibtexml;
    extern const QString dtdBibtexml;

    extern const uint syntaxVersion;
    extern const QString nsTellico;

    QString pubTellico(int version = syntaxVersion);
    QString dtdTellico(int version = syntaxVersion);

    bool versionConversion(uint from, uint to);

    extern const QString nsBookcase;
    extern const QString nsDublinCore;
    extern const QString nsZing;
    extern const QString nsZingDiag;

    bool validXMLElementName(const QString& name);
    QString elementName(const QString& name);
  }
}

#endif
