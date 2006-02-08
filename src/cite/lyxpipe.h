/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : $EMAIL
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_CITE_LYXPIPE_H
#define TELLICO_CITE_LYXPIPE_H

#include "actionmanager.h"

namespace Tellico {
  namespace Cite {

/**
 * @author Robby Stephenson
 */
class Lyxpipe : public Action {
public:
  Lyxpipe();

  virtual CiteAction type() const { return CiteClipboard; }
  virtual bool cite(Data::EntryVec entries);
};

  } // end namespace
} // end namespace

#endif
