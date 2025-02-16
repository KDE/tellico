/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "fetchertest.h"

#include "../fetch/fetchmanager.h"
#include "../fetch/fetcherinitializer.h"

#include <KLocalizedString>

#include <QTest>
#include <QSignalSpy>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( FetcherTest )

void FetcherTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::Fetch::FetcherInitializer initFetchers;
}

void FetcherTest::testType() {
  auto list = Tellico::Fetch::Manager::self()->fetchers();
  QVERIFY(!list.isEmpty());

  auto registry = Tellico::Fetch::Manager::self()->functionRegistry;
  auto i = registry.constBegin();
  auto end = registry.constEnd();
  for( ; i != end; ++i) {
    auto f = registry.value(i.key()).create(this);
    QVERIFY2(i.key() == f->type(),
             qPrintable(QString::fromLatin1("%1 has mismatched type: %2 != %3").arg(f->source(),
                                                                                    QString::number(i.key()),
                                                                                    QString::number(f->type()))));
    QVERIFY(!i.value().name().isEmpty());
    QVERIFY(f->uuid().isEmpty());
    QVERIFY(!f->isSearching());
    QVERIFY(!f->hasMoreResults());
    QVERIFY(!f->canFetch(Tellico::Fetch::FetchLast)); // invalid

    Tellico::Data::Collection::Type cType = Tellico::Data::Collection::Base;
    // BoardGame is the last collection type (currently)
    while(!f->canFetch(cType) && cType <= Tellico::Data::Collection::BoardGame) {
      cType = static_cast<Tellico::Data::Collection::Type>(cType+1);
    }
    if(cType > Tellico::Data::Collection::BoardGame) {
      // no point in trying now. Fetchers like ExecExternal won't match
      continue;
    }
    QSignalSpy doneSpy(f.data(), &Tellico::Fetch::Fetcher::signalDone);
    QSignalSpy resultSpy(f.data(), &Tellico::Fetch::Fetcher::signalResultFound);
    // test invalid search request key
    Tellico::Fetch::FetchRequest req(cType, Tellico::Fetch::FetchLast, QString());
    f->startSearch(req);
    QCOMPARE(doneSpy.count(), 1);
    QCOMPARE(resultSpy.count(), 0);
  }
}
