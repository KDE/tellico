/***************************************************************************
    copyright            : (C) 2008-2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "importer.h"
#include "../tellico_debug.h"

#include <klocale.h>

using Tellico::Import::Importer;

Tellico::Data::CollPtr Importer::currentCollection() const {
  if(!m_currentCollection) {
    myWarning() << "set current collection pointer!" << endl;
  }
  return m_currentCollection;
}

QString Importer::progressLabel() const {
  return url().isEmpty() ?
         i18n("Loading data...") :
         i18n("Loading %1...", url().fileName());
}

#include "importer.moc"
