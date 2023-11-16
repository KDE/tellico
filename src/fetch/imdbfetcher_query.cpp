/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
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

#include "imdbfetcher.h"

namespace {
  static const uint IMDB_MAX_RESULTS = 20;
  static const int IMDB_MAX_PERSON_COUNT = 5; // limit number of directors, writers, etc, esp for TV series
}

using Tellico::Fetch::IMDBFetcher;

QString IMDBFetcher::searchQuery() {
  static const auto query(QStringLiteral(R"(
query Search($searchTerms: String!) {
  mainSearch(first: %1, options: {searchTerm: $searchTerms, isExactMatch: false, type: TITLE}) {
    edges {
      node {
        entity {
          ... on Title {
            id
            titleText {
              text
            }
            titleType {
              text
            }
            releaseYear {
              year
              endYear
            }
          }
        }
      }
    }
  }
}
)"));
  return query.arg(IMDB_MAX_RESULTS);
}

QString IMDBFetcher::titleQuery() {
  static const auto query(QStringLiteral(R"(
query TitleFull($id: ID!) {
#  __type(name: "Company") {
#    name
#    fields {
#      name
#      type {
#        name
#        kind
#      }
#    }
#  }

  title(id: $id) {
    titleText {
      text
    }
    titleType {
      text
    }
    originalTitleText {
      text
    }
    canonicalUrl
    akas(first: 99) {
      edges {
        node {
          text
        }
      }
    }
    releaseYear {
      year
    }
    runtime {
      seconds
    }
    countriesOfOrigin {
      countries {
        text
      }
    }
    spokenLanguages {
      spokenLanguages {
        text
      }
    }
    ratingsSummary {
      aggregateRating
    }
    certificate {
      rating
      country {
        text
      }
    }
    plot {
      plotText {
        plainText
      }
    }
    genres {
      genres {
        text
      }
    }
    primaryImage {
      url
      width
      height
    }
    companyCredits(first: 99, filter: {categories: ["production"]}) {
      edges {
        node {
          category {
            text
            id
          }
          company {
            companyText {
              text
            }
          }
        }
      }
    }
    technicalSpecifications {
      colorations {
        items {
          text
        }
      }
      soundMixes {
        items {
          text
        }
      }
      aspectRatios {
        items {
          aspectRatio
        }
      }
    }
    cast: credits(first: 99, filter: {categories: ["cast"]}) {
      edges {
        node {
          name {
            nameText {
              text
            }
          }
          ... on Cast {
            characters {
              name
            }
          }
        }
      }
    }
    producers: credits(first: 99, filter: {categories: ["producer"]}) {
      edges {
        node {
          name {
            nameText {
              text
            }
          }
        }
      }
    }
    directors: credits(first: 99, filter: {categories: ["director"]}) {
      edges {
        node {
          name {
            nameText {
              text
            }
          }
        }
      }
    }
    writers: credits(first: 99, filter: {categories: ["writer"]}) {
      edges {
        node {
          name {
            nameText {
              text
            }
          }
        }
      }
    }
    composers: credits(first: 99, filter: {categories: ["composer"]}) {
      edges {
        node {
          name {
            nameText {
              text
            }
          }
        }
      }
    }
    principalProducers: principalCredits(filter: {categories: ["producer"]}) {
      ...Credits
    }
    principalDirectors: principalCredits(filter: {categories: ["director"]}) {
      ...Credits
    }
  }
}
fragment Credits on PrincipalCreditsForCategory {
  credits(limit: 99) {
    name {
      nameText {
        text
      }
    }
  }
}
)"));
  return query;
}

QString IMDBFetcher::episodeQuery() {
  static const auto query(QStringLiteral(R"(
query TitleFull($id: ID!) {
  title(id: $id) {
    titleText {
      text
    }
    titleType {
      text
    }
    originalTitleText {
      text
    }
    canonicalUrl
    akas(first: 9999) {
      edges {
        node {
          text
        }
      }
    }
    releaseYear {
      year
    }
    countriesOfOrigin {
      countries {
        text
      }
    }
    spokenLanguages {
      spokenLanguages {
        text
      }
    }
    ratingsSummary {
      aggregateRating
    }
    certificate {
      rating
      country {
        text
      }
    }
    plot {
      plotText {
        plainText
      }
    }
    genres {
      genres {
        text
      }
    }
    primaryImage {
      url
      width
      height
    }
    companyCredits(first: 99, filter: {categories: ["production"]}) {
      edges {
        node {
          category {
            text
            id
          }
          company {
            companyText {
              text
            }
          }
        }
      }
    }
    technicalSpecifications {
      colorations {
        items {
          text
        }
      }
      soundMixes {
        items {
          text
        }
      }
      aspectRatios {
        items {
          aspectRatio
        }
      }
    }
    cast: credits(first: 99, filter: {categories: ["cast"]}) {
      edges {
        node {
          name {
            nameText {
              text
            }
          }
          ... on Cast {
            characters {
              name
            }
          }
        }
      }
    }
    producers: credits(first: %1, filter: {categories: ["producer"]}) {
      edges {
        node {
          name {
            nameText {
              text
            }
          }
        }
      }
    }
    directors: credits(first: %1, filter: {categories: ["director"]}) {
      edges {
        node {
          name {
            nameText {
              text
            }
          }
        }
      }
    }
    writers: credits(first: %1, filter: {categories: ["writer"]}) {
      edges {
        node {
          name {
            nameText {
              text
            }
          }
        }
      }
    }
    composers: credits(first: %1, filter: {categories: ["composer"]}) {
      edges {
        node {
          name {
            nameText {
              text
            }
          }
        }
      }
    }
    principalProducers: principalCredits(filter: {categories: ["producer"]}) {
      ...Credits
    }
    principalDirectors: principalCredits(filter: {categories: ["director"]}) {
      ...Credits
    }
    episodes {
      episodes(first: 999) {
        edges {
          node {
            # Get the names of the episodes
            titleText {
              text
            }
            series {
              displayableEpisodeNumber {
                displayableSeason {
                  text
                }
                episodeNumber {
                  text
                }
              }
            }
          }
        }
      }
    }
  }
}
fragment Credits on PrincipalCreditsForCategory {
  credits(limit: %1) {
    name {
      nameText {
        text
      }
    }
  }
}
)"));
  return query.arg(IMDB_MAX_PERSON_COUNT);
}
