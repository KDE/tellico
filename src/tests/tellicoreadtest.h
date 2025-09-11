/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICOREADTEST_H
#define TELLICOREADTEST_H

#include <QObject>
#include <QList>

#include "../collection.h"

class TellicoReadTest : public QObject {
Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void init();

  void testBookCollection();
  void testEntries();
  void testEntries_data();
  void testCoinCollection();
  void testBibtexCollection();
  void testTableData();
  void testDuplicateLoans();
  void testDuplicateBorrowers();
  void testLocalImage();
  void testLocalImageLink();
  void testRemoteImage();
  void testRemoteImageLink();
  void testDataImage();
  void testXMLHandler();
  void testXMLHandler_data();
  void testXmlName();
  void testXmlName_data();
  void testRecoverXmlName();
  void testRecoverXmlName_data();
  void testBug418067();
  void testNoCreationDate();
  void testFutureVersion();
  void testRelativeLink();
  void testEmptyFirstTableRow();
  void testBug443845();
  void testEmoji();
  void testXmlWithJunk();
  void testRemote();
  void testImageLocation();
  void testSmallFile();

private:
  QList<Tellico::Data::CollPtr> m_collections;
};

#endif
