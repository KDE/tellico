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

#include "textimporter.h"
#include "../filehandler.h"

using Tellico::Import::TextImporter;

TextImporter::TextImporter(const KURL& url_, bool useUTF8_)
    : Import::Importer(url_.isValid() ? FileHandler::readTextFile(url_, false, useUTF8_) : QString()) {
}

TextImporter::TextImporter(const QString& text_) : Import::Importer(text_) {
}

#include "textimporter.moc"
