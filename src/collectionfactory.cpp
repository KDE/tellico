/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "collectionfactory.h"
#include "collections/bookcollection.h"
#include "collections/bibtexcollection.h"
#include "collections/musiccollection.h"
#include "collections/videocollection.h"
#include "collections/comicbookcollection.h"
#include "collections/coincollection.h"
#include "collections/stampcollection.h"
#include "collections/cardcollection.h"
#include "collections/winecollection.h"
#include "collections/gamecollection.h"
#include "collections/filecatalog.h"
#include "collections/boardgamecollection.h"
#include "field.h"
#include "tellico_debug.h"

#include <klocale.h>

using Tellico::CollectionFactory;

// static
Tellico::Data::CollPtr CollectionFactory::collection(int type_, bool addFields_) {
  switch(type_) {
    case Data::Collection::Book:
      return Data::CollPtr(new Data::BookCollection(addFields_));

    case Data::Collection::Video:
      return Data::CollPtr(new Data::VideoCollection(addFields_));

    case Data::Collection::Album:
      return Data::CollPtr(new Data::MusicCollection(addFields_));

    case Data::Collection::Bibtex:
      return Data::CollPtr(new Data::BibtexCollection(addFields_));

    case Data::Collection::Coin:
      return Data::CollPtr(new Data::CoinCollection(addFields_));

    case Data::Collection::Card:
      return Data::CollPtr(new Data::CardCollection(addFields_));

    case Data::Collection::Stamp:
      return Data::CollPtr(new Data::StampCollection(addFields_));

    case Data::Collection::Wine:
      return Data::CollPtr(new Data::WineCollection(addFields_));

    case Data::Collection::ComicBook:
      return Data::CollPtr(new Data::ComicBookCollection(addFields_));

    case Data::Collection::Game:
      return Data::CollPtr(new Data::GameCollection(addFields_));

    case Data::Collection::File:
      return Data::CollPtr(new Data::FileCatalog(addFields_));

    case Data::Collection::BoardGame:
      return Data::CollPtr(new Data::BoardGameCollection(addFields_));

    case Data::Collection::Base:
      break;

    default:
      kWarning() << "CollectionFactory::collection() - collection type not implemented: " << type_;
      // fall through
  }

  Data::CollPtr c(new Data::Collection(i18n("My Collection")));
  Data::FieldPtr f(new Data::Field(QLatin1String("title"), i18n("Title")));
  f->setCategory(i18n("General"));
  f->setFlags(Data::Field::NoDelete);
  f->setFormatFlag(Data::Field::FormatTitle);
  c->addField(f);
  return c;
}

// static
Tellico::Data::CollPtr CollectionFactory::collection(const QString& typeName_, bool addFields_) {
  if(typeName_ == typeName(Data::Collection::Book)) {
    return Data::CollPtr(new Data::BookCollection(addFields_));
  } else if(typeName_ == typeName(Data::Collection::Album)) {
    return Data::CollPtr(new Data::MusicCollection(addFields_));
  } else if(typeName_ == typeName(Data::Collection::Video)) {
    return Data::CollPtr(new Data::VideoCollection(addFields_));
  } else if(typeName_ == typeName(Data::Collection::Bibtex)) {
    return Data::CollPtr(new Data::BibtexCollection(addFields_));
  } else if(typeName_ == typeName(Data::Collection::Bibtex)) {
    return Data::CollPtr(new Data::ComicBookCollection(addFields_));
  } else if(typeName_ == typeName(Data::Collection::ComicBook)) {
    return Data::CollPtr(new Data::CardCollection(addFields_));
  } else if(typeName_ == typeName(Data::Collection::Coin)) {
    return Data::CollPtr(new Data::CoinCollection(addFields_));
  } else if(typeName_ == typeName(Data::Collection::Stamp)) {
    return Data::CollPtr(new Data::StampCollection(addFields_));
  } else if(typeName_ == typeName(Data::Collection::Wine)) {
    return Data::CollPtr(new Data::WineCollection(addFields_));
  } else if(typeName_ == typeName(Data::Collection::Game)) {
    return Data::CollPtr(new Data::GameCollection(addFields_));
  } else if(typeName_ == typeName(Data::Collection::File)) {
    return Data::CollPtr(new Data::FileCatalog(addFields_));
  } else if(typeName_ == typeName(Data::Collection::BoardGame)) {
    return Data::CollPtr(new Data::BoardGameCollection(addFields_));
  } else {
    kWarning() << "CollectionFactory::collection() - collection type not implemented: " << typeName_;
    return Data::CollPtr();
  }
}

Tellico::CollectionNameMap CollectionFactory::nameMap() {
  CollectionNameMap map;
  map[Data::Collection::Book]        = i18n("Book Collection");
  map[Data::Collection::Bibtex]      = i18n("Bibliography");
  map[Data::Collection::ComicBook]   = i18n("Comic Book Collection");
  map[Data::Collection::Video]       = i18n("Video Collection");
  map[Data::Collection::Album]       = i18n("Music Collection");
  map[Data::Collection::Coin]        = i18n("Coin Collection");
  map[Data::Collection::Stamp]       = i18n("Stamp Collection");
  map[Data::Collection::Wine]        = i18n("Wine Collection");
  map[Data::Collection::Card]        = i18n("Card Collection");
  map[Data::Collection::Game]        = i18n("Game Collection");
  map[Data::Collection::File]        = i18n("File Catalog");
  map[Data::Collection::BoardGame]   = i18n("Board Game Collection");
  map[Data::Collection::Base]        = i18n("Custom Collection");
  return map;
}

QString CollectionFactory::typeName(int type_) {
  switch(type_) {
    case Data::Collection::Book:
      return QLatin1String("book");
      break;

    case Data::Collection::Video:
      return QLatin1String("video");
      break;

    case Data::Collection::Album:
      return QLatin1String("album");
      break;

    case Data::Collection::Bibtex:
      return QLatin1String("bibtex");
      break;

    case Data::Collection::ComicBook:
      return QLatin1String("comic");
      break;

    case Data::Collection::Wine:
      return QLatin1String("wine");
      break;

    case Data::Collection::Coin:
      return QLatin1String("coin");
      break;

    case Data::Collection::Stamp:
      return QLatin1String("stamp");
      break;

    case Data::Collection::Card:
      return QLatin1String("card");
      break;

    case Data::Collection::Game:
      return QLatin1String("game");
      break;

    case Data::Collection::File:
      return QLatin1String("file");
      break;

    case Data::Collection::BoardGame:
      return QLatin1String("boardgame");
      break;

    case Data::Collection::Base:
      return QLatin1String("entry");
      break;

    default:
      kWarning() << "CollectionFactory::collection() - collection type not implemented: " << type_;
      return QLatin1String("entry");
      break;
  }
}

bool CollectionFactory::isDefaultField(int type_, const QString& name_) {
  Data::CollPtr coll = collection(type_, true);
  foreach(Data::FieldPtr field, coll->fields()) {
    if(field->name() == name_) {
      return true;
    }
  }
  return false;
}
