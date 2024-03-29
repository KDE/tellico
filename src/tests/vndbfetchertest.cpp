/***************************************************************************
    Copyright (C) 2013-2020 Robby Stephenson <robby@periapsis.org>
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

#include "../fetch/vndbfetcher.h"
#include "../collections/gamecollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../images/imagefactory.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( VNDBFetcherTest )

VNDBFetcherTest::VNDBFetcherTest() : AbstractFetcherTest() {
}

void VNDBFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::GameCollection> registerGame(Tellico::Data::Collection::Game, "game");
  Tellico::ImageFactory::init();

  m_config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("vndb"));
  m_config.writeEntry("Custom Fields", QStringLiteral("origtitle,alias"));

  m_fieldValues.insert(QStringLiteral("title"), QStringLiteral("G-senjou no Maou"));
  m_fieldValues.insert(QStringLiteral("year"), QStringLiteral("2008"));
  m_fieldValues.insert(QStringLiteral("developer"), QStringLiteral("AkabeiSoft2"));
  m_fieldValues.insert(QStringLiteral("publisher"), QStringLiteral("AkabeiSoft2"));
  m_fieldValues.insert(QStringLiteral("origtitle"), QStringLiteral("G線上の魔王"));
  m_fieldValues.insert(QStringLiteral("alias"), QStringLiteral("The Devil on G-String"));
}

void VNDBFetcherTest::testTitle() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Game, Tellico::Fetch::Title,
                                       QStringLiteral("G-senjou no Maou"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::VNDBFetcher(this));
  fetcher->readConfig(m_config);

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  Tellico::Data::EntryPtr entry = results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    if(i.key() == QLatin1String("alias")) {
      QEXPECT_FAIL("", "alias value was removed from the vndb data for this item", Continue);
    }
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QStringLiteral("description")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
}
