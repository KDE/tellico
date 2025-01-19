/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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

#include "completiontest.h"

#include "../fieldcompletion.h"
#include "../field.h"

#include <QTest>

QTEST_APPLESS_MAIN( CompletionTest )

void CompletionTest::testBasic() {
  Tellico::FieldCompletion cmp(false); // not multiple
  cmp.addItem(QStringLiteral("Atlas V"));
  cmp.addItem(QStringLiteral("Falcon 9"));
  cmp.addItem(QStringLiteral("New Shephard"));
  cmp.addItem(QStringLiteral("New Glenn"));

  QString result = cmp.makeCompletion(QStringLiteral("Fal"));
  QCOMPARE(result, QStringLiteral("Falcon 9"));
}

void CompletionTest::testMultiple() {
  Tellico::FieldCompletion cmp(true); // multiple
  cmp.addItem(QStringLiteral("Atlas V"));
  cmp.addItem(QStringLiteral("Falcon 9"));
  cmp.addItem(QStringLiteral("New Shephard"));
  cmp.addItem(QStringLiteral("New Glenn"));
  cmp.addItem(QStringLiteral("Vulcan"));
  cmp.addItem(QStringLiteral("Starship"));

  QString result = cmp.makeCompletion(QStringLiteral("Fal"));
  QCOMPARE(result, QStringLiteral("Falcon 9"));
  result = cmp.makeCompletion(QStringLiteral("Atlas V; Fal"));
  QCOMPARE(result, QStringLiteral("Atlas V; Falcon 9"));
  QVERIFY(!cmp.hasMultipleMatches());
  result = cmp.makeCompletion(QStringLiteral("Atlas V; New"));
  // matches first inserted
  QCOMPARE(result, QStringLiteral("Atlas V; New Shephard"));
  QVERIFY(cmp.hasMultipleMatches());
  QStringList allMatches = cmp.allMatches();
  QCOMPARE(allMatches.count(), 2);
  QVERIFY(allMatches.contains(QStringLiteral("Atlas V; New Shephard")));
  QVERIFY(allMatches.contains(QStringLiteral("Atlas V; New Glenn")));

  // complete on third item
  result = cmp.makeCompletion(QStringLiteral("Atlas V; Falcon 9; S"));
  QCOMPARE(result, QStringLiteral("Atlas V; Falcon 9; Starship"));

  // now back to a single match
  result = cmp.makeCompletion(QStringLiteral("Atl"));
  QCOMPARE(result, QStringLiteral("Atlas V"));
}
