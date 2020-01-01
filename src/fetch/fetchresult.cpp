/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "fetchresult.h"
#include "fetcher.h"
#include "../entry.h"
#include "../collection.h"
#include "../tellico_debug.h"

#include <KRandom>

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

using namespace Tellico;
using namespace Tellico::Fetch;
using Tellico::Fetch::FetchResult;

FetchResult::FetchResult(Fetcher::Ptr fetcher_, Data::EntryPtr entry_)
   : uid(KRandom::random())
   , fetcher(fetcher_)
   , title(entry_->title())
   , desc(makeDescription(entry_))
   , isbn(entry_->field(QStringLiteral("isbn"))) {
}

FetchResult::FetchResult(Fetcher::Ptr fetcher_, const QString& title_, const QString& desc_, const QString& isbn_)
   : uid(KRandom::random())
   , fetcher(fetcher_)
   , title(title_)
   , desc(desc_)
   , isbn(isbn_) {
}

Tellico::Data::EntryPtr FetchResult::fetchEntry() {
  return fetcher->fetchEntry(uid);
}

QString FetchResult::makeDescription(Data::EntryPtr entry) {
  Q_ASSERT(entry);
  QString desc;
  switch(entry->collection()->type()) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      append(desc, entry, "author");
      append(desc, entry, "publisher");
      append(desc, entry, "cr_year") || append(desc, entry, "pub_year") || append(desc, entry, "year");
      append(desc, entry, "issue");
      break;

    case Data::Collection::ComicBook:
      append(desc, entry, "series");
      append(desc, entry, "issue");
      append(desc, entry, "publisher");
      append(desc, entry, "pub_year") || append(desc, entry, "year");
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
      break;

    case Data::Collection::Wine:
      append(desc, entry, "appellation");
      break;

    case Data::Collection::Coin:
      append(desc, entry, "country");
      append(desc, entry, "description");
      break;

    default:
      myDebug() << "no description for collection type =" << entry->collection()->type();
      break;
  }

  return desc;
}
