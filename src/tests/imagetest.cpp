/***************************************************************************
    Copyright (C) 2009-2021 Robby Stephenson <robby@periapsis.org>
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

#include "imagetest.h"

#include "../images/imagefactory.h"
#include "../images/image.h"

#include <KLocalizedString>

#include <QTest>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( ImageTest )

void ImageTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::ImageFactory::init();
}

void ImageTest::testLinkOnly() {
  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("../../icons/128-apps-tellico.png"));
  // addImage(url, quiet, referer, link)
  QString id = Tellico::ImageFactory::addImage(u, false, QUrl(), true);
  QCOMPARE(id, u.url());

  const auto img1 = Tellico::ImageFactory::imageById(id);
  QVERIFY(!img1.isNull());
  QCOMPARE(img1.format(), "png");
  QVERIFY(img1.linkOnly());
  QCOMPARE(img1.width(), 128);

  const auto img2 = img1;
  QCOMPARE(img1, img2);
  QCOMPARE(img1.id(), img2.id());
  QCOMPARE(img1.linkOnly(), img2.linkOnly());

  auto null = Tellico::Data::Image::null;
  QVERIFY(null.isNull());
  QVERIFY(null.id().isEmpty());
  QVERIFY(!null.linkOnly());
}

void ImageTest::testOrientation() {
  QUrl u1 = QUrl::fromLocalFile(QFINDTESTDATA("data/img1.jpg"));
  QString id1 = Tellico::ImageFactory::addImage(u1);
  const auto img1 = Tellico::ImageFactory::imageById(id1);
  QRgb px = img1.pixel(0, 0);
  QVERIFY(qRed(px) > 250 && qGreen(px) < 5 && qBlue(px) < 5);

  QUrl u2 = QUrl::fromLocalFile(QFINDTESTDATA("data/img2.jpg"));
  QString id2 = Tellico::ImageFactory::addImage(u2);
  const auto img2 = Tellico::ImageFactory::imageById(id2);
  px = img2.pixel(0, 0);
  QVERIFY(qRed(px) > 250 && qGreen(px) < 5 && qBlue(px) < 5);
}
