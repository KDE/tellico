/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_H
#define TELLICO_FETCH_H

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
  UPC,
  Keyword,
  DOI,
  ArxivID,
  PubmedID,
  LCCN,
  Raw,
  ExecUpdate,
  FetchLast
};

// real ones must start at 0!
enum Type {
  Unknown = -1,
  Amazon = 0,
  IMDB,
  Z3950,
  SRU,
  Entrez,
  ExecExternal,
  Yahoo, // Removed
  AnimeNfo, // Removed
  IBS,
  ISBNdb,
  GCstarPlugin,
  CrossRef,
  Citebase, // Removed
  Arxiv,
  Bibsonomy,
  GoogleScholar,
  Discogs,
  WineCom,
  TheMovieDB,
  MusicBrainz,
  GiantBomb,
  OpenLibrary,
  Multiple,
  Freebase, // Removed
  DVDFr,
  Filmaster,
  Douban,
  BiblioShare,
  MovieMeter,
  GoogleBook,
  MAS, // Removed
  Springer,
  Allocine, // Removed
  ScreenRush, // Removed
  FilmStarts, // Removed
  SensaCine, // Removed
  Beyazperde, // Removed
  HathiTrust,
  TheGamesDB,
  DBLP,
  VNDB,
  MRLookup,
  BoardGameGeek,
  Bedetheque,
  OMDB,
  KinoPoisk,
  VideoGameGeek,
  DBC,
  IGDB,
  Kino,
  MobyGames,
  ComicVine,
  KinoTeatr,
  Colnect,
  Numista,
  TVmaze,
  UPCItemDb,
  TheTVDB,
  RPGGeek,
  GamingHistory,
  FilmAffinity,
  Itunes,
  OPDS,
  ADS,
  VGCollect
};

  }
}

#endif
