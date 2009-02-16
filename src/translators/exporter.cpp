/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "exporter.h"
#include "../document.h"
#include "../collection.h"

using Tellico::Export::Exporter;

Exporter::Exporter() : QObject(), m_options(Export::ExportUTF8 | Export::ExportComplete) {
}

Exporter::Exporter(Tellico::Data::CollPtr coll) : QObject(), m_options(Export::ExportUTF8), m_coll(coll) {
}

Exporter::~Exporter() {
}

Tellico::Data::CollPtr Exporter::collection() const {
  if(m_coll) {
    return m_coll;
  }
  return Data::Document::self()->collection();
}

#include "exporter.moc"
