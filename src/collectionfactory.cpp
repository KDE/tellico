/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "field.h"

#include <kdebug.h>

using Tellico::CollectionFactory;

// static
Tellico::Data::CollPtr CollectionFactory::collection(Data::Collection::Type type_, bool addFields_, const QString& entryTitle_) {
  switch(type_) {
    case Data::Collection::Book:
      return new Data::BookCollection(addFields_);

    case Data::Collection::Video:
      return new Data::VideoCollection(addFields_);

    case Data::Collection::Album:
      return new Data::MusicCollection(addFields_);

    case Data::Collection::Bibtex:
      return new Data::BibtexCollection(addFields_);

    case Data::Collection::Coin:
      return new Data::CoinCollection(addFields_);

    case Data::Collection::Card:
      return new Data::CardCollection(addFields_);

    case Data::Collection::Stamp:
      return new Data::StampCollection(addFields_);

    case Data::Collection::Wine:
      return new Data::WineCollection(addFields_);

    case Data::Collection::ComicBook:
      return new Data::ComicBookCollection(addFields_);

    case Data::Collection::Game:
      return new Data::GameCollection(addFields_);

    case Data::Collection::File:
      return new Data::FileCatalog(addFields_);

    case Data::Collection::Base:
      break; // fall through and create custom collection

    default:
      kdWarning() << "CollectionFactory::collection() - collection type not implemented: " << type_ << endl;
      // fall through
  }

  Data::CollPtr c = new Data::Collection(i18n("My Collection"), entryTitle_);
  Data::FieldPtr f = new Data::Field(QString::fromLatin1("title"), i18n("Title"));
  f->setCategory(i18n("General"));
  f->setFlags(Data::Field::NoDelete);
  f->setFormatFlag(Data::Field::FormatTitle);
  c->addField(f);
  return c;
}

// static
Tellico::Data::CollPtr CollectionFactory::collection(const QString& typeName_, bool addFields_) {
  if(typeName_ == typeName(Data::Collection::Book)) {
    return new Data::BookCollection(addFields_);
  } else if(typeName_ == typeName(Data::Collection::Album)) {
    return new Data::MusicCollection(addFields_);
  } else if(typeName_ == typeName(Data::Collection::Video)) {
    return new Data::VideoCollection(addFields_);
  } else if(typeName_ == typeName(Data::Collection::Bibtex)) {
    return new Data::BibtexCollection(addFields_);
  } else if(typeName_ == typeName(Data::Collection::Bibtex)) {
    return new Data::ComicBookCollection(addFields_);
  } else if(typeName_ == typeName(Data::Collection::ComicBook)) {
    return new Data::CardCollection(addFields_);
  } else if(typeName_ == typeName(Data::Collection::Coin)) {
    return new Data::CoinCollection(addFields_);
  } else if(typeName_ == typeName(Data::Collection::Stamp)) {
    return new Data::StampCollection(addFields_);
  } else if(typeName_ == typeName(Data::Collection::Wine)) {
    return new Data::WineCollection(addFields_);
  } else if(typeName_ == typeName(Data::Collection::Game)) {
    return new Data::GameCollection(addFields_);
  } else if(typeName_ == typeName(Data::Collection::File)) {
    return new Data::FileCatalog(addFields_);
  } else {
    kdWarning() << "CollectionFactory::collection() - collection type not implemented: " << typeName_ << endl;
    return 0;
  }
}

Tellico::CollectionNameMap CollectionFactory::nameMap() {
  CollectionNameMap map;
  map[Data::Collection::Book]   = i18n("Book Collection");
  map[Data::Collection::Bibtex] = i18n("Bibliography");
  map[Data::Collection::Coin]   = i18n("Comic Book Collection");
  map[Data::Collection::Video]  = i18n("Video Collection");
  map[Data::Collection::Album]  = i18n("Music Collection");
  map[Data::Collection::Coin]   = i18n("Coin Collection");
  map[Data::Collection::Stamp]  = i18n("Stamp Collection");
  map[Data::Collection::Wine]   = i18n("Wine Collection");
  map[Data::Collection::Card]   = i18n("Card Collection");
  map[Data::Collection::Game]   = i18n("Game Collection");
  map[Data::Collection::File]   = i18n("File Catalog");
  map[Data::Collection::Base]   = i18n("Custom Collection");
  return map;
}

QString CollectionFactory::typeName(Data::Collection::Type type_) {
  switch(type_) {
    case Data::Collection::Book:
      return QString::fromLatin1("book");
      break;

    case Data::Collection::Video:
      return QString::fromLatin1("video");
      break;

    case Data::Collection::Album:
      return QString::fromLatin1("album");
      break;

    case Data::Collection::Bibtex:
      return QString::fromLatin1("bibtex");
      break;

    case Data::Collection::Coin:
      return QString::fromLatin1("coin");
      break;

    case Data::Collection::Card:
      return QString::fromLatin1("card");
      break;

    case Data::Collection::Stamp:
      return QString::fromLatin1("stamp");
      break;

    case Data::Collection::Wine:
      return QString::fromLatin1("wine");
      break;

    case Data::Collection::ComicBook:
      return QString::fromLatin1("comic");
      break;

    case Data::Collection::Game:
      return QString::fromLatin1("game");
      break;

    case Data::Collection::File:
      return QString::fromLatin1("file");
      break;

    case Data::Collection::Base:
      return QString::fromLatin1("entry");
      break;

    default:
      kdWarning() << "CollectionFactory::collection() - collection type not implemented: " << type_ << endl;
      return QString::fromLatin1("entry");
      break;
  }
}

