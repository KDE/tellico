/***************************************************************************
    Copyright (C) 2024 Robby Stephenson <robby@periapsis.org>
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

#include "rtftest.h"
#include "../3rdparty/rtf2html/rtf2html.h"

#include <QTest>
#include <QFile>
#include <QTextStream>

QTEST_APPLESS_MAIN( RtfTest )

void RtfTest::initTestCase() {
}

void RtfTest::testRtf() {
  QFile rtfFile(QFINDTESTDATA("data/rtf2html-input.rtf"));
  QVERIFY(rtfFile.open(QIODevice::ReadOnly));
  QTextStream input(&rtfFile);

  QFile htmlFile(QFINDTESTDATA("data/rtf2html-output.html"));
  QVERIFY(htmlFile.open(QIODevice::ReadOnly));
  QTextStream output(&htmlFile);

  const QString rtf = input.readAll();
  const QString html = output.readAll();

  Tellico::RTF2HTML rtf2html(rtf);
  QCOMPARE(rtf2html.toHTML(), html);
}
