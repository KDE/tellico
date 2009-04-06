/***************************************************************************
    copyright            : (C) 2003-2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FETCH_SEARCHRESULT_H
#define TELLICO_FETCH_SEARCHRESULT_H

#include "fetcher.h"

#include <QString>

namespace Tellico {
  namespace Fetch {

class SearchResult {
public:
  SearchResult(Fetcher::Ptr f, Data::EntryPtr entry);
  SearchResult(Fetcher::Ptr f, const QString& t, const QString& d);
  SearchResult(Fetcher::Ptr f, const QString& t, const QString& d, const QString& i);

  Data::EntryPtr fetchEntry();

  uint uid;
  Fetcher::Ptr fetcher;
  QString title;
  QString desc;
  QString isbn;

private:
  static QString makeDescription(Data::EntryPtr entry);
};

  } // end namespace
} // end namespace

#endif
