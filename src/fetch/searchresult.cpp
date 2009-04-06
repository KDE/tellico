/***************************************************************************
    copyright            : (C) 2005-20089 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "searchresult.h"
#include "fetcher.h"
#include "../entry.h"
#include "../collection.h"
#include "../tellico_debug.h"

#include <krandom.h>

namespace {
  bool append(QString& text, Tellico::Data::EntryPtr entry, const char* field) {
    const QString value = entry->field(QLatin1String(field));
    if(value.isEmpty()) {
      return false;
    }
    if(!text.isEmpty()) {
      text += QLatin1Char('/');
    }
    text += value;
    return true;
  }
}

using Tellico::Fetch::SearchResult;

SearchResult::SearchResult(Fetcher::Ptr fetcher_, Data::EntryPtr entry_)
   : uid(KRandom::random())
   , fetcher(fetcher_)
   , title(entry_->title())
   , desc(makeDescription(entry_))
   , isbn(entry_->field(QLatin1String("isbn"))) {
}

SearchResult::SearchResult(Fetcher::Ptr fetcher_, const QString& title_, const QString& desc_)
   : uid(KRandom::random())
   , fetcher(fetcher_)
   , title(title_)
   , desc(desc_) {
}

SearchResult::SearchResult(Fetcher::Ptr fetcher_, const QString& title_, const QString& desc_, const QString& isbn_)
   : uid(KRandom::random())
   , fetcher(fetcher_)
   , title(title_)
   , desc(desc_)
   , isbn(isbn_) {
}

Tellico::Data::EntryPtr SearchResult::fetchEntry() {
  return fetcher->fetchEntry(uid);
}

QString SearchResult::makeDescription(Data::EntryPtr entry) {
  Q_ASSERT(entry);
  QString desc;
  switch(entry->collection()->type()) {
    case Data::Collection::Book:
    case Data::Collection::ComicBook:
    case Data::Collection::Bibtex:
      append(desc, entry, "author");
      append(desc, entry, "publisher");
      append(desc, entry, "cr_year") || append(desc, entry, "pub_year") || append(desc, entry, "year");
      break;

    case Data::Collection::Video:
      append(desc, entry, "studio");
      append(desc, entry, "director");
      append(desc, entry, "year");
      append(desc, entry, "medium");
      break;

    case Data::Collection::Album:
      append(desc, entry, "artist");
      append(desc, entry, "label");
      append(desc, entry, "year");
      break;

    case Data::Collection::Game:
      append(desc, entry, "platform");
      append(desc, entry, "year");
      break;

    case Data::Collection::BoardGame:
      append(desc, entry, "publisher");
      append(desc, entry, "designer");
      append(desc, entry, "year");

    default:
      myDebug() << "no description for collection type =" << entry->collection()->type();
      break;
  }

  return desc;
}
