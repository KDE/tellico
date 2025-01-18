/***************************************************************************
    Copyright (C) 2019-2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef COLNECTFETCHERTEST_H
#define COLNECTFETCHERTEST_H

#include "abstractfetchertest.h"

#include <KConfigGroup>

class ColnectFetcherTest : public AbstractFetcherTest {
Q_OBJECT
public:
  ColnectFetcherTest();

private Q_SLOTS:
  void initTestCase();
  void testSlug();
  void testSlug_data();
  void testRaw();
  void testSacagawea();
  void testSkylab();
  void testComic();
  void testComicIsbn();
  void testBaseballCard();
  void testGoldeneye();

private:
  KConfigGroup m_config;
};

#endif
