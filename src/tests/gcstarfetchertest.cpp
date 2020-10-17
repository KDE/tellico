/***************************************************************************
    Copyright (C) 2010-2011 Robby Stephenson <robby@periapsis.org>
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

#include "gcstarfetchertest.h"

#include "../fetch/gcstarpluginfetcher.h"
#include "../entry.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../fieldformat.h"
#include "../utils/datafileregistry.h"

#include <KSharedConfig>
#include <KConfigGroup>

#include <QTest>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( GCstarFetcherTest )

GCstarFetcherTest::GCstarFetcherTest() : AbstractFetcherTest() {
}

void GCstarFetcherTest::initTestCase() {
  const QString gcstar = QStandardPaths::findExecutable(QStringLiteral("gcstar"));
  if(gcstar.isEmpty()) {
    QSKIP("This test requires gcstar", SkipAll);
  }

  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");

  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/gcstar2tellico.xsl"));
}

void GCstarFetcherTest::testSnowyRiver() {
  KConfigGroup cg = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("gcstar"));
  cg.writeEntry("CollectionType", QStringLiteral("3"));
  cg.writeEntry("Plugin", QStringLiteral("Amazon (US)"));

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Superman Returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::GCstarPluginFetcher(this));
  fetcher->readConfig(cg);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QStringLiteral("Superman Returns"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!Tellico::ImageFactory::imageById(entry->field("cover")).isNull());
}
