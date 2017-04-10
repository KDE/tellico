/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#undef QT_NO_CAST_FROM_ASCII

#include "dbcfetchertest.h"

#include "../fetch/dbcfetcher.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../utils/datafileregistry.h"

#include <QTest>

QTEST_GUILESS_MAIN( DBCFetcherTest )

DBCFetcherTest::DBCFetcherTest() : AbstractFetcherTest() {
}

void DBCFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/dbc2tellico.xsl"));
}

void DBCFetcherTest::testIsbn() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::ISBN,
                                       QLatin1String("9788711391839"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::DBCFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QCOMPARE(entry->field(QLatin1String("title")), QLatin1String("Min kamp"));
  QCOMPARE(entry->field(QLatin1String("author")), QString::fromUtf8("Karl Ove Knausgård"));
  QCOMPARE(entry->field(QLatin1String("publisher")), QLatin1String("Lindhardt og Ringhof"));
  QCOMPARE(entry->field(QLatin1String("year")), QLatin1String("2012"));
  QCOMPARE(entry->field(QLatin1String("isbn")), QLatin1String("978-87-11-39183-9"));
  QCOMPARE(entry->field(QLatin1String("pages")), QLatin1String("487"));
}
