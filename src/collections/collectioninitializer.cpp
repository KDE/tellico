/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "collectioninitializer.h"
#include "../collectionfactory.h"
#include "bibtexcollection.h"
#include "boardgamecollection.h"
#include "bookcollection.h"
#include "cardcollection.h"
#include "coincollection.h"
#include "comicbookcollection.h"
#include "filecatalog.h"
#include "gamecollection.h"
#include "musiccollection.h"
#include "stampcollection.h"
#include "videocollection.h"
#include "winecollection.h"

/**
 * Ideally, I'd like these initializations to be in each cpp file for each collection type
 * but as a static variable, they weren't always bein initialized, so do it the manual way.
 *
 * But, at the least, collectionfactory is not coupled to every single collection type
 */
Tellico::CollectionInitializer::CollectionInitializer() {
  RegisterCollection<Data::Collection>          registerBase(Data::Collection::Base,           "entry");
  RegisterCollection<Data::BibtexCollection>    registerBibtex(Data::Collection::Bibtex,       "bibtex");
  RegisterCollection<Data::BoardGameCollection> registerBoardGame(Data::Collection::BoardGame, "boardgame");
  RegisterCollection<Data::BookCollection>      registerBook(Data::Collection::Book,           "book");
  RegisterCollection<Data::CardCollection>      registerCard(Data::Collection::Card,           "card");
  RegisterCollection<Data::CoinCollection>      registerCoin(Data::Collection::Coin,           "coin");
  RegisterCollection<Data::ComicBookCollection> registerComic(Data::Collection::ComicBook,     "comic");
  RegisterCollection<Data::FileCatalog>         registerFile(Data::Collection::File,           "file");
  RegisterCollection<Data::GameCollection>      registerGame(Data::Collection::Game,           "game");
  RegisterCollection<Data::MusicCollection>     registerMusic(Data::Collection::Album,         "album");
  RegisterCollection<Data::StampCollection>     registerStamp(Data::Collection::Stamp,         "stamp");
  RegisterCollection<Data::VideoCollection>     registerVideo(Data::Collection::Video,         "video");
  RegisterCollection<Data::WineCollection>      registerWine(Data::Collection::Wine,           "wine");
}
