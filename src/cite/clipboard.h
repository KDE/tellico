/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_CITE_CLIPBOARD_H
#define TELLICO_CITE_CLIPBOARD_H

#include "actionmanager.h"

namespace Tellico {
  namespace Cite {

/**
 * @author Robby Stephenson
*/
class Clipboard : public Action {
public:
  Clipboard();

  virtual CiteAction type() const { return CiteClipboard; }
  virtual bool cite(Data::EntryList entries);
};

  }
}

#endif
