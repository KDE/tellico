/***************************************************************************
    Copyright (C) 2009-2011 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>

#include "fetcherinitializer.h"
#include "amazonfetcher.h"
#include "imdbfetcher.h"
#ifdef HAVE_YAZ
#include "z3950fetcher.h"
#endif
#include "srufetcher.h"
#include "entrezfetcher.h"
#include "execexternalfetcher.h"
#include "animenfofetcher.h"
#include "ibsfetcher.h"
#include "isbndbfetcher.h"
#include "gcstarpluginfetcher.h"
#include "crossreffetcher.h"
#include "arxivfetcher.h"
#include "bibsonomyfetcher.h"
#include "googlescholarfetcher.h"
#include "discogsfetcher.h"
#include "winecomfetcher.h"
#include "themoviedbfetcher.h"
#include "musicbrainzfetcher.h"
#include "giantbombfetcher.h"
#include "openlibraryfetcher.h"
#include "multifetcher.h"
#include "freebasefetcher.h"
#include "filmasterfetcher.h"
#include "hathitrustfetcher.h"
#include "vndbfetcher.h"
#include "dvdfrfetcher.h"
#include "doubanfetcher.h"
#include "bibliosharefetcher.h"
#include "moviemeterfetcher.h"
#include "googlebookfetcher.h"
#include "springerfetcher.h"
#include "allocinefetcher.h"
#include "thegamesdbfetcher.h"
#include "dblpfetcher.h"
#include "mrlookupfetcher.h"
#include "boardgamegeekfetcher.h"
#include "bedethequefetcher.h"
#include "omdbfetcher.h"
#include "kinopoiskfetcher.h"
#include "videogamegeekfetcher.h"
#include "dbcfetcher.h"
#include "igdbfetcher.h"

/**
 * Ideally, I'd like these initializations to be in each cpp file for each collection type
 * but as a static variable, they weren't always being initialized, so do it the manual way.
 */
Tellico::Fetch::FetcherInitializer::FetcherInitializer() {
#ifdef ENABLE_AMAZON
  RegisterFetcher<Fetch::AmazonFetcher> registerAmazon(Amazon);
#endif
#ifdef ENABLE_IMDB
  RegisterFetcher<Fetch::IMDBFetcher> registerIMDB(IMDB);
#endif
#ifdef HAVE_YAZ
  RegisterFetcher<Fetch::Z3950Fetcher> registerZ3950(Z3950);
#endif
  RegisterFetcher<Fetch::SRUFetcher> registerSRU(SRU);
  RegisterFetcher<Fetch::EntrezFetcher> registerEntrez(Entrez);
  RegisterFetcher<Fetch::ExecExternalFetcher> registerExternal(ExecExternal);
  RegisterFetcher<Fetch::AnimeNfoFetcher> registerAnimeNfo(AnimeNfo);
  RegisterFetcher<Fetch::IBSFetcher> registerIBS(IBS);
  RegisterFetcher<Fetch::ISBNdbFetcher> registerISBNdb(ISBNdb);
  RegisterFetcher<Fetch::GCstarPluginFetcher> registerGCstar(GCstarPlugin);
  RegisterFetcher<Fetch::CrossRefFetcher> registerCrossRef(CrossRef);
  RegisterFetcher<Fetch::ArxivFetcher> registerArxiv(Arxiv);
  RegisterFetcher<Fetch::BibsonomyFetcher> registerBibsonomy(Bibsonomy);
  RegisterFetcher<Fetch::GoogleScholarFetcher> registerGoogle(GoogleScholar);
  RegisterFetcher<Fetch::WineComFetcher> registerWine(WineCom);
  RegisterFetcher<Fetch::MusicBrainzFetcher> registerMB(MusicBrainz);
  RegisterFetcher<Fetch::GiantBombFetcher> registerBomb(GiantBomb);
  RegisterFetcher<Fetch::OpenLibraryFetcher> registerOpenLibrary(OpenLibrary);
  RegisterFetcher<Fetch::MultiFetcher> registerMulti(Multiple);
  RegisterFetcher<Fetch::DiscogsFetcher> registerDiscogs(Discogs);
  RegisterFetcher<Fetch::TheMovieDBFetcher> registerTMDB(TheMovieDB);
  RegisterFetcher<Fetch::FreebaseFetcher> registerFreebase(Freebase);
  RegisterFetcher<Fetch::FilmasterFetcher> registerFilmaster(Filmaster);
  RegisterFetcher<Fetch::GoogleBookFetcher> registerGoogleBook(GoogleBook);
  RegisterFetcher<Fetch::HathiTrustFetcher> registerHathiTrust(HathiTrust);
  RegisterFetcher<Fetch::VNDBFetcher> registerVNDB(VNDB);
  RegisterFetcher<Fetch::AllocineFetcher> registerAllocine(Allocine);
  RegisterFetcher<Fetch::MovieMeterFetcher> registerMovieMeter(MovieMeter);
  RegisterFetcher<Fetch::DVDFrFetcher> registerDVDFr(DVDFr);
  RegisterFetcher<Fetch::DoubanFetcher> registerDouban(Douban);
  RegisterFetcher<Fetch::BiblioShareFetcher> registerBiblioShare(BiblioShare);
  RegisterFetcher<Fetch::SpringerFetcher> registerSpringer(Springer);
  RegisterFetcher<Fetch::TheGamesDBFetcher> registerTheGamesDB(TheGamesDB);
  RegisterFetcher<Fetch::DBLPFetcher> registerDBLP(DBLP);
  RegisterFetcher<Fetch::MRLookupFetcher> registerMRLookup(MRLookup);
  RegisterFetcher<Fetch::BoardGameGeekFetcher> registerBGG(BoardGameGeek);
  RegisterFetcher<Fetch::BedethequeFetcher> registerBD(Bedetheque);
  RegisterFetcher<Fetch::OMDBFetcher> registerOMDB(OMDB);
  RegisterFetcher<Fetch::KinoPoiskFetcher> registerKinoPoisk(KinoPoisk);
  RegisterFetcher<Fetch::VideoGameGeekFetcher> registerVGG(VideoGameGeek);
  RegisterFetcher<Fetch::DBCFetcher> registerDBC(DBC);
  RegisterFetcher<Fetch::IGDBFetcher> registerIGDB(IGDB);

  Fetch::Manager::self()->loadFetchers();
}
