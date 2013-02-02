/***************************************************************************
    Copyright (C) 2013 Robby Stephenson <robby@periapsis.org>
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

#include "vndbfetchertest.h"
#include "vndbfetchertest.moc"
#include "qtest_kde.h"

#include "../fetch/vndbfetcher.h"
#include "../collections/gamecollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KConfigGroup>

QTEST_KDEMAIN( VNDBFetcherTest, GUI )

VNDBFetcherTest::VNDBFetcherTest() : AbstractFetcherTest() {
}

void VNDBFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::GameCollection> registerGame(Tellico::Data::Collection::Game, "game");
  Tellico::ImageFactory::init();

  m_fieldValues.insert(QLatin1String("title"), QLatin1String("G-senjou no Maou"));
  m_fieldValues.insert(QLatin1String("year"), QLatin1String("2008"));
//  m_fieldValues.insert(QLatin1String("developer"), QLatin1String("Akabei Soft2"));
  m_fieldValues.insert(QLatin1String("origtitle"), QString::fromUtf8("G-線上の魔王"));
  m_fieldValues.insert(QLatin1String("alias"), QLatin1String("The Devil on G-String"));
}

void VNDBFetcherTest::testTitle() {
  KConfig config(QString::fromLatin1(KDESRCDIR) + QLatin1String("/tellicotest.config"), KConfig::SimpleConfig);
  QString groupName = QLatin1String("vndb");
  if(!config.hasGroup(groupName)) {
    QSKIP("This test requires a config file.", SkipAll);
  }
  KConfigGroup cg(&config, groupName);

  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Title,
                                       QLatin1String("G-senjou no Maou"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::VNDBFetcher(this));
  fetcher->readConfig(cg, cg.name());

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QLatin1String("description")).isEmpty());
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}
