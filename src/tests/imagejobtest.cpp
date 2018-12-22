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
#include "../images/imageinfo.h"

#include <QTest>
#include <QEventLoop>
#include <QTemporaryFile>
#include <QNetworkInterface>
#include <QSignalSpy>

QTEST_GUILESS_MAIN( ImageJobTest )

bool ImageJobTest::networkIsAvailable() {
  foreach(const QNetworkInterface& net, QNetworkInterface::allInterfaces()) {
    if(net.flags().testFlag(QNetworkInterface::IsUp) && !net.flags().testFlag(QNetworkInterface::IsLoopBack)) {
      return true;
    }
  }
  return false;
}

void ImageJobTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void ImageJobTest::cleanupTestCase() {
}

void ImageJobTest::init() {
  m_result = -1;
  m_imageId.clear();
}

void ImageJobTest::enterLoop() {
  QEventLoop eventLoop;
  connect(this, &ImageJobTest::exitLoop, &eventLoop, &QEventLoop::quit);
  eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

void ImageJobTest::slotGetResult(KJob* job) {
  m_result = job->error();
  emit exitLoop();
}

void ImageJobTest::slotAvailable(const QString& id_) {
  m_result = 0;
  m_imageId = id_;
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
  QUrl u(QStringLiteral("file:///non-existent-location"));

  Tellico::ImageJob* job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  QCOMPARE(m_result, int(KIO::ERR_CANNOT_OPEN_FOR_READING));
}

void ImageJobTest::testUnreadable() {
  QTemporaryFile tmpFile;
  QVERIFY(tmpFile.open());
  QVERIFY(tmpFile.setPermissions(QFileDevice::Permissions()));
  QUrl u = QUrl::fromLocalFile(tmpFile.fileName());

  Tellico::ImageJob* job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  QCOMPARE(m_result, int(KIO::ERR_CANNOT_OPEN_FOR_READING));

  const Tellico::Data::Image& img = job->image();
  QVERIFY(img.isNull());
}

void ImageJobTest::testImageInvalid() {
  // text file is an invalid image
  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("imagejobtest.cpp"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  QCOMPARE(m_result, int(KIO::ERR_UNKNOWN));

  const Tellico::Data::Image& img = job->image();
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

  const Tellico::Data::Image& img = job->image();
  QVERIFY(!img.isNull());
  QCOMPARE(img.id(), QStringLiteral("dde5bf2cbd90fad8635a26dfb362e0ff.png"));
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), false);

  // check that the job is automatically deleted
  qApp->processEvents();
  QVERIFY(!job);
}

void ImageJobTest::testImageLoadWithId() {
  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("../../icons/tellico.png"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u, QStringLiteral("tellico-rocks"));
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  // success!
  QCOMPARE(m_result, 0);

  const Tellico::Data::Image& img = job->image();
  QVERIFY(!img.isNull());
  QCOMPARE(img.id(), QStringLiteral("tellico-rocks"));
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), false);
}

void ImageJobTest::testImageLink() {
  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("../../icons/tellico.png"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u,
                                                          QString() /* id */,
                                                          false /* quiet */);
  job->setLinkOnly(true);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  // success!
  QCOMPARE(m_result, 0);

  const Tellico::Data::Image& img = job->image();
  QVERIFY(!img.isNull());
  // id is not the MD5 hash
  QVERIFY(img.id() != QStringLiteral("dde5bf2cbd90fad8635a26dfb362e0ff.png"));
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), true);
}

void ImageJobTest::testNetworkImage() {
  if(!networkIsAvailable()) {
    QSKIP("This test requires network access", SkipSingle);
    return;
  }

  QUrl u(QStringLiteral("http://tellico-project.org/sites/default/files/logo.png"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  // success!
  QCOMPARE(m_result, 0);

  const Tellico::Data::Image& img = job->image();
  QVERIFY(!img.isNull());
  QCOMPARE(img.id(), QStringLiteral("757322046f4aa54290a3d92b05b71ca1.png"));
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), false);

  // check that the job is automatically deleted
  qApp->processEvents();
  QVERIFY(!job);
}

void ImageJobTest::testNetworkImageLink() {
  if(!networkIsAvailable()) {
    QSKIP("This test requires network access", SkipSingle);
    return;
  }

  QUrl u(QStringLiteral("http://tellico-project.org/sites/default/files/logo.png"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u,
                                                          QString() /* id */,
                                                          false /* quiet */);
  job->setLinkOnly(true);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  // success!
  QCOMPARE(m_result, 0);

  const Tellico::Data::Image& img = job->image();
  QVERIFY(!img.isNull());
  QCOMPARE(img.id(), u.url());
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), true);
}

void ImageJobTest::testNetworkImageInvalid() {
  if(!networkIsAvailable()) {
    QSKIP("This test requires network access", SkipSingle);
    return;
  }

  QUrl u(QStringLiteral("http://tellico-project.org"));

  QPointer<Tellico::ImageJob> job = new Tellico::ImageJob(u);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(slotGetResult(KJob*)));

  enterLoop();
  QCOMPARE(m_result, int(KIO::ERR_UNKNOWN));

  const Tellico::Data::Image& img = job->image();
  QVERIFY(img.isNull());
}

void ImageJobTest::testFactoryRequestLocal() {
  QVERIFY(m_imageId.isEmpty());
  connect(Tellico::ImageFactory::self(), &Tellico::ImageFactory::imageAvailable,
          this, &ImageJobTest::slotAvailable);

  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("../../icons/tellico.png"));
  Tellico::ImageFactory::requestImageById(u.url());

  // don't need to enter loop since the image is local and signal fires immediately
  QVERIFY(!m_imageId.isEmpty());
  // success!
  QCOMPARE(m_result, 0);

  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(m_imageId);
  QVERIFY(!img.isNull());
  // id is not the MD5 hash
  QVERIFY(img.id() != QStringLiteral("dde5bf2cbd90fad8635a26dfb362e0ff.png"));
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), true);
}

void ImageJobTest::testFactoryRequestLocalInvalid() {
  QVERIFY(m_imageId.isEmpty());
  QSignalSpy spy(Tellico::ImageFactory::self(), &Tellico::ImageFactory::imageAvailable);
  connect(Tellico::ImageFactory::self(), &Tellico::ImageFactory::imageAvailable,
          this, &ImageJobTest::slotAvailable);

  // text file is an invalid image
  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("imagejobtest.cpp"));
  Tellico::ImageFactory::requestImageById(u.url());

  // it will be a null image, but a local url, so image is still loaded with immediate signal
  QCOMPARE(spy.count(), 1);
  // the available image id is the url
  QCOMPARE(m_imageId, u.url());

  // now try to load it
  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(m_imageId);
  QVERIFY(img.isNull());
  QCOMPARE(img.linkOnly(), false);
  // make sure the null image list is updated
  QVERIFY(Tellico::ImageFactory::self()->hasNullImage(m_imageId));
  // the image should not be in local memory now
  QVERIFY(!Tellico::ImageFactory::self()->hasImageInMemory(m_imageId));
  QVERIFY(!Tellico::ImageFactory::self()->hasImageInfo(m_imageId));
}

void ImageJobTest::testFactoryRequestNetwork() {
  if(!networkIsAvailable()) {
    QSKIP("This test requires network access", SkipSingle);
    return;
  }

  QVERIFY(m_imageId.isEmpty());
  connect(Tellico::ImageFactory::self(), &Tellico::ImageFactory::imageAvailable,
          this, &ImageJobTest::slotAvailable);

  QUrl u(QStringLiteral("http://tellico-project.org/sites/default/files/logo.png"));
  Tellico::ImageFactory::requestImageById(u.url());

  enterLoop();
  QVERIFY(!m_imageId.isEmpty());
  // success!
  QCOMPARE(m_result, 0);
  // the image should be in local memory now
  QVERIFY(Tellico::ImageFactory::self()->hasImageInMemory(m_imageId));
  QVERIFY(Tellico::ImageFactory::self()->hasImageInfo(m_imageId));

  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(m_imageId);
  QVERIFY(!img.isNull());
  // id is the MD5 hash, since it's not link only
  QCOMPARE(img.id(), QStringLiteral("757322046f4aa54290a3d92b05b71ca1.png"));
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), false);
}

void ImageJobTest::testFactoryRequestNetworkLinkOnly() {
  if(!networkIsAvailable()) {
    QSKIP("This test requires network access", SkipSingle);
    return;
  }

  QVERIFY(m_imageId.isEmpty());
  connect(Tellico::ImageFactory::self(), &Tellico::ImageFactory::imageAvailable,
          this, &ImageJobTest::slotAvailable);

  QUrl u(QStringLiteral("http://tellico-project.org/sites/default/files/logo.png"));
  // first, tell the image factory that the image is link only
  Tellico::Data::ImageInfo info(u.url(), "PNG", 64, 64, true /* link only */);
  Tellico::ImageFactory::cacheImageInfo(info);

  Tellico::ImageFactory::requestImageById(u.url());

  enterLoop();
  QVERIFY(!m_imageId.isEmpty());
  // success!
  QCOMPARE(m_result, 0);
  // the image should be in local memory now
  QVERIFY(Tellico::ImageFactory::self()->hasImageInMemory(m_imageId));
  QVERIFY(Tellico::ImageFactory::self()->hasImageInfo(m_imageId));

  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(m_imageId);
  QVERIFY(!img.isNull());
  // id is not the MD5 hash
  QCOMPARE(img.id(), u.url());
  QCOMPARE(img.format(), QByteArray("png"));
  QCOMPARE(img.linkOnly(), true);
}
