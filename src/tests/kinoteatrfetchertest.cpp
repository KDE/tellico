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

#include "kinoteatrfetchertest.h"

#include "../fetch/kinoteatrfetcher.h"
#include "../entry.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../fieldformat.h"

#include <KSharedConfig>

#include <QTest>

QTEST_GUILESS_MAIN( KinoTeatrFetcherTest )

KinoTeatrFetcherTest::KinoTeatrFetcherTest() : AbstractFetcherTest() {
}

void KinoTeatrFetcherTest::initTestCase() {
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");

  m_config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig)->group(QStringLiteral("kinoteatr"));
  m_config.writeEntry("Custom Fields", QStringLiteral("origtitle,kinoteatr"));
}

void KinoTeatrFetcherTest::testSuperman() {
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Video, Tellico::Fetch::Title, QStringLiteral("Superman Returns"));
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::KinoTeatrFetcher(this));
  fetcher->readConfig(m_config);
  QVERIFY(fetcher->canSearch(request.key()));

  Tellico::Data::EntryList results = DO_FETCH1(fetcher, request, 1);

  QCOMPARE(results.size(), 1);

  // the first entry had better be the right one
  Tellico::Data::EntryPtr entry = results.at(0);

  QCOMPARE(entry->field("title"), QString::fromUtf8("Супермен повертається"));
  QCOMPARE(entry->field("origtitle"), QStringLiteral("Superman Returns"));
  QCOMPARE(entry->field("year"), QStringLiteral("2006"));
  QCOMPARE(entry->field("nationality"), QString::fromUtf8("США"));
  QCOMPARE(entry->field("director"), QString::fromUtf8("Брайан Сінґер"));
  QCOMPARE(entry->field("writer"), QString::fromUtf8("Брайан Сінґер; Майкл Доґерті"));
  QCOMPARE(entry->field("producer"), QString::fromUtf8("Гілберт Адлер"));
  QCOMPARE(entry->field("composer"), QString::fromUtf8("Джон Оттмен"));
  QCOMPARE(set(entry, "genre"), set(QString::fromUtf8("бойовик; пригоди; фантастика; фентезі")));
  QCOMPARE(entry->field("running-time"), QStringLiteral("154"));
  QStringList castList = Tellico::FieldFormat::splitTable(entry->field(QStringLiteral("cast")));
  QVERIFY(!castList.isEmpty());
  QCOMPARE(castList.at(0), QString::fromUtf8("Джеймс Марсден::Річард Вайт"));
  QVERIFY(entry->field("plot").startsWith(QString::fromUtf8("Легендарний супер-герой")));
  QVERIFY(!entry->field("cover").isEmpty());
  QVERIFY(!entry->field(QStringLiteral("cover")).contains(QLatin1Char('/')));
  QCOMPARE(entry->field("kinoteatr"), QStringLiteral("https://kino-teatr.ua/uk/film/superman-returns-3475.phtml"));
}
