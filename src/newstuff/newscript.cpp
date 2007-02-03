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

#include <qwidget.h>

using Tellico::NewStuff::NewScript;

NewScript::NewScript(Manager* manager_, QWidget* parentWidget_)
#if KDE_IS_VERSION(3,3,90)
    : KNewStuffSecure(QString::fromLatin1("tellico/data-source"), parentWidget_)
#else
    : QObject(parentWidget_)
#endif
    , m_manager(manager_), m_success(false) {
}

void NewScript::installResource() {
  // m_tarName is protected in superclass
  KURL u;
  u.setPath(m_tarName);
  m_success = m_manager->installScript(u);
  m_url = u;
}

#if KDE_IS_VERSION(3,3,90)
#include <knewstuff/knewstuffsecure.h>
#define SUPERCLASS KNewStuffSecure
#else
#define SUPERCLASS QObject
#endif

#include "newscript.moc"
#undef SUPERCLASS
