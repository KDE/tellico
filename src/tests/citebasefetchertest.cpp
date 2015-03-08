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

#undef QT_NO_CAST_FROM_ASCII

#include "citebasefetchertest.h"
#include "qtest_kde.h"

#include "../fetch/citebasefetcher.h"
#include "../entry.h"
#include "../collections/bibtexcollection.h"
#include "../collectionfactory.h"

#include <KStandardDirs>

QTEST_KDEMAIN( CitebaseFetcherTest, GUI )

CitebaseFetcherTest::CitebaseFetcherTest() : AbstractFetcherTest() {
}

void CitebaseFetcherTest::initTestCase() {
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  // since citebase uses the bibtex importer
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../translators/");
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");

  m_fieldValues.insert(QLatin1String("arxiv"), QLatin1String("hep-lat/0110180"));
  m_fieldValues.insert(QLatin1String("entry-type"), QLatin1String("article"));
  m_fieldValues.insert(QLatin1String("title"), QLatin1String("Speeding up the Hybrid-Monte-Carlo algorithm for dynamical fermions"));
  m_fieldValues.insert(QLatin1String("author"), QString::fromUtf8("M.  Hasenbusch; K.  Jansen"));
//  m_fieldValues.insert(QLatin1String("journal"), QLatin1String("Nuclear Physics B - Proceedings Supplements"));
  m_fieldValues.insert(QLatin1String("journal"), QLatin1String("NUCL.PHYS.PROC."));
  m_fieldValues.insert(QLatin1String("year"), QLatin1String("2002"));
  m_fieldValues.insert(QLatin1String("volume"), QLatin1String("106"));
  // should really be 1076-1078, but citebase seems to have it wrong
  m_fieldValues.insert(QLatin1String("pages"), QLatin1String("1076"));
}

void CitebaseFetcherTest::testArxivID() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Bibtex, Tellico::Fetch::ArxivID,
                                       "arxiv:" + m_fieldValues.value("arxiv"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::CitebaseFetcher(this));

  Tellico::Data::EntryList results = DO_FETCH(fetcher, request);

  QEXPECT_FAIL("", "Citebase has gone away and redirects to ADS", Abort);
  QCOMPARE(results.size(), 1);

  if(!results.isEmpty()) {
    Tellico::Data::EntryPtr entry = results.at(0);
    QHashIterator<QString, QString> i(m_fieldValues);
    while(i.hasNext()) {
      i.next();
      QCOMPARE(entry->field(i.key()), i.value());
    }
  }
}
