/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "kernel.h"
#include "document.h"
#include "collection.h"

using Bookcase::Kernel;

Kernel* Kernel::s_self = 0;

Kernel::Kernel(QWidget* parent_, const char* name_/*=0*/) : QObject(parent_, name_) {
  m_doc = new Data::Document(this);
}

Bookcase::Data::Collection* Kernel::collection() {
  return m_doc->collection();
}

const QStringList& Kernel::fieldTitles() const {
  return m_doc->collection()->fieldTitles();
}

QString Kernel::fieldNameByTitle(const QString& title_) const {
  return m_doc->collection()->fieldNameByTitle(title_);
}

QString Kernel::fieldTitleByName(const QString& name_) const {
  return m_doc->collection()->fieldTitleByName(name_);
}

#include "kernel.moc"
