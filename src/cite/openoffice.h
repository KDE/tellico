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

#ifndef TELLICO_CITE_OPENOFFICE_H
#define TELLICO_CITE_OPENOFFICE_H

#include "actionmanager.h"

namespace Tellico {
  namespace Cite {

/**
 * @author Robby Stephenson
 */
class OpenOffice : public Action {
public:
  OpenOffice();
  ~OpenOffice();

  virtual CiteAction type() const { return CiteOpenOffice; }

  virtual bool connect();
  virtual bool cite(Data::EntryList entries);
  virtual State state() const;

  static bool hasLibrary();

private:
  bool connectionDialog();
  class Private;
  Private* d;
};

  }
}

#endif
