/***************************************************************************
                           bccollectionfactory.cpp
                             -------------------
    begin                : Fri Sep 12 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bccollectionfactory.h"
#include "collections/bookcollection.h"
#include "collections/bibtexcollection.h"
#include "collections/musiccollection.h"
#include "collections/videocollection.h"
//#include "collections/comicbookcollection.h"
//#include "collections/coincollection.h"
//#include "collections/stampcollection.h"
//#include "collections/cardcollection.h"
//#include "collections/winecollection.h"

#include <kdebug.h>

// static
BCCollection* BCCollectionFactory::collection(BCCollection::CollectionType type_, bool addAttributes_) {
  switch(type_) {
    case BCCollection::Book:
      return new BookCollection(addAttributes_);
      break;

    case BCCollection::Video:
      return new VideoCollection(addAttributes_);
      break;

    case BCCollection::Album:
      return new MusicCollection(addAttributes_);
      break;

    case BCCollection::Bibtex:
      return new BibtexCollection(addAttributes_);
      break;

    case BCCollection::Card:
    case BCCollection::Coin:
    case BCCollection::Stamp:
    case BCCollection::Wine:
    case BCCollection::ComicBook:
    case BCCollection::Base:
    default:
      kdWarning() << "BCCollectionFactory::collection() - collection type not implemented: " << type_ << endl;
      return new BookCollection(addAttributes_);
      break;
  }
}

// static
BCCollection* BCCollectionFactory::collection(const QString& unitName_, bool addAttributes_) {
  if(unitName_== QString::fromLatin1("book")) {
    return new BookCollection(addAttributes_);
  } else if(unitName_== QString::fromLatin1("album")) {
    return new MusicCollection(addAttributes_);
  } else if(unitName_== QString::fromLatin1("video")) {
    return new VideoCollection(addAttributes_);
  } else if(unitName_== QString::fromLatin1("entry")) {
    return new BibtexCollection(addAttributes_);
//  } else if(unitName_== QString::fromLatin1("comic")) {
//    return new ComicBookCollection(addAttributes_);
//  } else if(unitName_== QString::fromLatin1("card")) {
//    return new CardCollection(addAttributes_);
//  } else if(unitName_== QString::fromLatin1("coin")) {
//    return new CoinCollection(addAttributes_);
//  } else if(unitName_== QString::fromLatin1("stamp")) {
//    return new StampCollection(addAttributes_);
//  } else if(unitName_== QString::fromLatin1("wine")) {
//    return new WineCollection(addAttributes_);
  } else {
    kdWarning() << "BCCollectionFactory::collection() - collection type not implemented: " << unitName_ << endl;
    return new BookCollection(addAttributes_);
  }
}

CollectionNameMap BCCollectionFactory::typeMap() {
  CollectionNameMap map;
  map[BCCollection::Book] = i18n("Book Collection");
  map[BCCollection::Bibtex] = i18n("Bibliography");
  map[BCCollection::Video] = i18n("Video Collection");
  map[BCCollection::Album] = i18n("Music Collection");
  return map;
}

