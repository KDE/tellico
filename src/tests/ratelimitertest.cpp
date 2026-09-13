/***************************************************************************
    Copyright (C) 2026 Robby Stephenson <robby@periapsis.org>
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
 ***************************************************************************/

#include "ratelimitertest.h"

#include "../fetch/ratelimiter.h"

#include <KIO/Job>

#include <QPointer>
#include <QTest>
#include <QTimer>
#include <QLoggingCategory>

#include <chrono>

using Tellico::Fetch::RateLimiter;
using namespace Qt::Literals::StringLiterals;
using namespace std::chrono_literals;

class TestJob : public KIO::Job {
public:
  explicit TestJob(bool finishOnResume = false)
      : m_finishOnResume(finishOnResume) {
  }
  void finish() { emitResult(); }
  int resumeCount() const { return m_resumeCount; }

protected:
  bool doResume() override {
    ++m_resumeCount;
    const bool success = KIO::Job::doResume();
    if(success && m_finishOnResume) {
      QTimer::singleShot(0, this, [this]() {
        finish();
      });
    }
    return success;
  }

private:
  int m_resumeCount{0};
  bool m_finishOnResume;
};

QTEST_GUILESS_MAIN( RateLimiterTest )

RateLimiterTest::RateLimiterTest() {
  QLoggingCategory::setFilterRules(QStringLiteral("tellico.debug = true\ntellico.info = true"));
}

void RateLimiterTest::startsNextJobAfterCompletion() {
  RateLimiter limiter({{QString(), 10, 1s}}, 1 /* max jobs */);
  auto job1 = new TestJob;
  auto job2 = new TestJob;

  QVERIFY(limiter.addJob(job1));
  QVERIFY(limiter.addJob(job2));
  QTRY_COMPARE(job1->resumeCount(), 1);
  QCOMPARE(job2->resumeCount(), 0); // not resumed yet

  job1->finish();
  QTRY_COMPARE(job2->resumeCount(), 1);
}

void RateLimiterTest::obeysMultipleTiers() {
  RateLimiter limiter({{u"1"_s, 2, 50ms},
                       {u"2"_s, 3, 200ms}}, 4);
  auto job1 = new TestJob;
  auto job2 = new TestJob;
  auto job3 = new TestJob;
  auto job4 = new TestJob;

  QVERIFY(limiter.addJob(job1));
  QVERIFY(limiter.addJob(job2));
  QVERIFY(limiter.addJob(job3));
  QVERIFY(limiter.addJob(job4));
  QTRY_COMPARE(job2->resumeCount(), 1);
  QCOMPARE(job3->resumeCount(), 0); // not started yet

  QTRY_COMPARE_WITH_TIMEOUT(job3->resumeCount(), 1, 150);
  QCOMPARE(job4->resumeCount(), 0);
  QTRY_COMPARE_WITH_TIMEOUT(job4->resumeCount(), 1, 300);
}

void RateLimiterTest::updatesTierFromServer() {
  RateLimiter limiter({{u"test"_s, 10, 1s}}, 2);
  auto job1 = new TestJob;
  auto job2 = new TestJob;
  const auto resetTime = QDateTime::currentDateTimeUtc().addMSecs(100);

  QVERIFY(limiter.updateBucket(u"test"_s, 20, 1, resetTime));

  // An older concurrent response must not increase the remaining count in
  // the same server window.
  QVERIFY(limiter.updateBucket(u"test"_s, 20, 2, resetTime));

  QVERIFY(limiter.addJob(job1));
  QVERIFY(limiter.addJob(job2));
  QTRY_COMPARE(job1->resumeCount(), 1);
  QCOMPARE(job2->resumeCount(), 0);

  QTRY_COMPARE_WITH_TIMEOUT(job2->resumeCount(), 1, 250);
}

void RateLimiterTest::executesJob() {
  RateLimiter limiter({{QString(), 1, 1s}});
  QPointer<TestJob> job = new TestJob(true);

  QVERIFY(limiter.execJob(job));
  QVERIFY(job);
  QCOMPARE(job->resumeCount(), 1);
}

void RateLimiterTest::executesJobFromResultSlot() {
  RateLimiter limiter({{QString(), 10, 1s}}, 1 /*max*/);
  auto job1 = new TestJob;
  bool success = false;

  // Connect before addJob() to reproduce a fetcher's result handler running
  // before any result handler installed by the limiter.
  connect(job1, &KJob::result, this, [&limiter, &success]() {
    auto job2 = new TestJob(true);

    // Prevent a broken implementation from hanging the test indefinitely.
    QTimer::singleShot(250, job2, [job2]() {
      job2->deleteLater();
    });

    success = limiter.execJob(job2);
  });

  QVERIFY(limiter.addJob(job1));
  QTRY_COMPARE(job1->resumeCount(), 1);

  job1->finish();

  QVERIFY(success);
}

void RateLimiterTest::ownsJobs() {
  auto limiter = new RateLimiter({{QString(), 1, 1h}});
  QPointer<TestJob> job1 = new TestJob;
  QPointer<TestJob> job2 = new TestJob;

  QVERIFY(limiter->addJob(job1));
  QVERIFY(limiter->addJob(job2));
  delete limiter;

  QVERIFY(job1.isNull());
  QVERIFY(job2.isNull());
}
