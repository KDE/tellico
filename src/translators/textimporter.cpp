/***************************************************************************
                              textimporter.cpp
                             -------------------
    begin                : Wed Sep 24 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "textimporter.h"
#include "../bcfilehandler.h"

TextImporter::TextImporter(const KURL& url_) : Importer(url_) {
  // TODO: error handling?
  if(url_.isValid()) {
    m_text = BCFileHandler::readFile(url_);
  }
}
