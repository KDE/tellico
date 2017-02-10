/***************************************************************************
    Copyright (C) 2010-2012 Robby Stephenson <robby@periapsis.org>
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

#ifndef ALLOCINEFETCHERTEST_H
#define ALLOCINEFETCHERTEST_H

#include <config.h>
#include "abstractfetchertest.h"

class AllocineFetcherTest : public AbstractFetcherTest {
Q_OBJECT
public:
  AllocineFetcherTest();

private Q_SLOTS:
  void initTestCase();
  void cleanupTestCase();

  void testTitle();
  void testTitleAccented();
  void testTitleAccentRemoved();
  void testPlotQuote();

#ifdef HAVE_QJSON
  void testTitleAPI();
  void testTitleAPIAccented();
  void testGhostDog();

  void testTitleScreenRush();
  void testTitleFilmStarts();
  void testTitleFilmStartsGerman();
  void testTitleSensaCineSpanish();
  void testTitleBeyazperdeTurkish();
#endif
};

#endif
