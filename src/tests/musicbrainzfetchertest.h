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

#ifndef MUSICBRAINZFETCHERTEST_H
#define MUSICBRAINZFETCHERTEST_H

#include "abstractfetchertest.h"

#include <QHash>

class MusicBrainzFetcherTest : public AbstractFetcherTest {
Q_OBJECT
public:
  MusicBrainzFetcherTest();

private Q_SLOTS:
  void initTestCase();
  void cleanup();
  void testTitle();
  void testPerson();
  void testACDC();
  void testKeyword();
  void testBug426560();
  void testCoverArt();
  void testSoundtrack();
  void testBarcode();
  void testCatno();
  void testMbid();
  void testMultiDisc();
  void testMultiDiscOldWay();
  void testUpdate();

private:
  QHash<QString, QString> m_fieldValues;
};

#endif
