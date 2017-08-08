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

#include "imagejobtest.h"

#include "../images/imagejob.h"
#include "../images/imagefactory.h"

#include <QTest>
#include <QEventLoop>
#include <QTemporaryFile>

QTEST_GUILESS_MAIN( ImageJobTest )

void ImageJobTest::initTestCase() {
}

void ImageJobTest::cleanupTestCase() {
}

void ImageJobTest::enterLoop() {
  QEventLoop eventLoop;
  connect(this, &ImageJobTest::exitLoop, &eventLoop, &QEventLoop::quit);
  eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

void ImageJobTest::slotGetResult(KJob *job) {
  m_result = job->error();
  emit exitLoop();
}

void ImageJobTest::testInvalidUrl() {
  QUrl u;

  Tellico::ImageJob* job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  QCOMPARE(m_result, int(KIO::ERR_MALFORMED_URL));
}

void ImageJobTest::testNonexistant() {
  QUrl u(QLatin1String("file:///non-existant-location"));

  Tellico::ImageJob* job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  QCOMPARE(m_result, int(KIO::ERR_CANNOT_OPEN_FOR_READING));
}

void ImageJobTest::testUnreadable() {
  QTemporaryFile tmpFile;
  QVERIFY(tmpFile.open());
  QVERIFY(tmpFile.setPermissions(0));
  QUrl u = QUrl::fromLocalFile(tmpFile.fileName());

  Tellico::ImageJob* job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  QCOMPARE(m_result, int(KIO::ERR_CANNOT_OPEN_FOR_READING));
}

void ImageJobTest::testImageInvalid() {
  // text file is an invalid image
  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("imagejobtest.cpp"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  QCOMPARE(m_result, int(KIO::ERR_UNKNOWN));

  Tellico::Data::Image img = job->image();
  QVERIFY(img.isNull());
  QCOMPARE(img.linkOnly(), false);
}

void ImageJobTest::testImageLoad() {
  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("../../icons/tellico.png"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  // success!
  QCOMPARE(m_result, 0);

  Tellico::Data::Image img = job->image();
  QVERIFY(!img.isNull());
  QCOMPARE(img.id(), QLatin1String("dde5bf2cbd90fad8635a26dfb362e0ff.png"));
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), false);

  // check that the job is automatically deleted
  qApp->processEvents();
  QVERIFY(!job);
}

void ImageJobTest::testImageLoadWithId() {
  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("../../icons/tellico.png"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u, QLatin1String("tellico-rocks"));
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  // success!
  QCOMPARE(m_result, 0);

  Tellico::Data::Image img = job->image();
  QVERIFY(!img.isNull());
  QCOMPARE(img.id(), QLatin1String("tellico-rocks"));
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), false);
}

void ImageJobTest::testImageLink() {
  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("../../icons/tellico.png"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u,
                                                          QString() /* id */,
                                                          false /* quiet */,
                                                          true /* link only */);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  // success!
  QCOMPARE(m_result, 0);

  Tellico::Data::Image img = job->image();
  QVERIFY(!img.isNull());
  // id is not the MD5 hash
  QVERIFY(img.id() != QLatin1String("dde5bf2cbd90fad8635a26dfb362e0ff.png"));
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), true);
}

void ImageJobTest::testNetworkImage() {
  QUrl u(QLatin1String("http://tellico-project.org/sites/default/files/logo.png"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  // success!
  QCOMPARE(m_result, 0);

  Tellico::Data::Image img = job->image();
  QVERIFY(!img.isNull());
  QCOMPARE(img.id(), QLatin1String("757322046f4aa54290a3d92b05b71ca1.png"));
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), false);

  // check that the job is automatically deleted
  qApp->processEvents();
  QVERIFY(!job);
}
