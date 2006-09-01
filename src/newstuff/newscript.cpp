/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "newscript.h"
#include "manager.h"

#include <kurl.h>

using Tellico::NewStuff::NewScript;

NewScript::NewScript(Manager* manager_, QWidget* parentWidget_)
    : KNewStuffSecure(QString::fromLatin1("tellico/data-source"), parentWidget_)
    , m_manager(manager_) {
}

void NewScript::installResource() {
  // m_tarName is protected in superclass
  KURL u;
  u.setPath(m_tarName);
  m_manager->installScript(u);
}

#include "newscript.moc"
