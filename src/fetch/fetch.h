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

#ifndef FETCH_H
#define FETCH_H

namespace Tellico {
  namespace Fetch {

/**
 * FetchFirst must be first, and the rest must follow consecutively in value.
 * FetchLast must be last!
 */
enum FetchKey {
  FetchFirst = 0,
  Title,
  Person,
  ISBN,
  Keyword,
  Raw,
  FetchLast
};

// real ones must start at 0!
enum Type {
  Unknown = -1,
  Amazon = 0,
  IMDB,
  Z3950,
  SRU
};

  }
}

#endif
