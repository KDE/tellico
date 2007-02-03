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

#ifndef TELLICO_NEWSTUFF_NEWSCRIPT_H
#define TELLICO_NEWSTUFF_NEWSCRIPT_H

#include <kdeversion.h>
#include <kurl.h>

#if KDE_IS_VERSION(3,3,90)
#include <knewstuff/knewstuffsecure.h>
#define SUPERCLASS KNewStuffSecure
#else
#define SUPERCLASS QObject
#endif

#include <qobject.h>

namespace Tellico {
  namespace NewStuff {

class Manager;

class NewScript : public SUPERCLASS {
Q_OBJECT

public:
  NewScript(Manager* manager, QWidget* parentWidget = 0);
  virtual ~NewScript() {}

  const KURL& url() const { return m_url; }
  bool successfulInstall() const { return m_success; }

private:
  virtual void installResource();

  Manager* m_manager;
#if !KDE_IS_VERSION(3,3,90)
  // KNewStuffSecure has a protected variable
  QString m_tarName;
#endif
  KURL m_url;
  bool m_success : 1;
};

  }
}

#undef SUPERCLASS
#endif
