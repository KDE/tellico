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
#include <KLocalizedString>

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
  KLocalizedString::setApplicationDomain("tellico");
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

void RateLimiterTest::parsesResponseHeaders() {
  QFETCH(int, format);
  QFETCH(QString, headers);
  QFETCH(int, remaining);

  RateLimiter::ResponseHeaders limHeaders{u"limit"_s,
                                          u"remaining"_s,
                                          u"reset"_s,
                                          static_cast<RateLimiter::ResetFormat>(format),
                                          60s};

  RateLimiter limiter({{u"test"_s, 10, 1h, limHeaders},
                       {u"unmapped"_s, 5, 1min}});
  limiter.updateFromResponse(headers, 200);
  QCOMPARE(limiter.bucketRemaining(u"test"_s), remaining);
  QCOMPARE(limiter.bucketRemaining(u"unmapped"_s), 5);
}

void RateLimiterTest::parsesResponseHeaders_data() {
  QTest::addColumn<int>("format");
  QTest::addColumn<QString>("headers");
  QTest::addColumn<int>("remaining");

  const int relative = static_cast<int>(RateLimiter::ResetFormat::DelaySeconds);
  const int absolute = static_cast<int>(RateLimiter::ResetFormat::UnixSeconds);
  const int fixed = static_cast<int>(RateLimiter::ResetFormat::FixedDelay);

  const auto fromNow = QString::number(QDateTime::currentSecsSinceEpoch() + 180);
  QTest::newRow("relative") << relative << u"LIMIT: 20\r\nRemaining: 3\r\nReset: 120\r\n"_s << 3;
  QTest::newRow("absolute") << absolute << u"limit: 20\nremaining: 4\nreset: "_s + fromNow << 4;
  QTest::newRow("fixed") << fixed << u"limit: 20\nremaining: 5"_s << 5;
  QTest::newRow("expired-absolute") << absolute << u"limit: 20\nremaining: 0\nreset: 1"_s << 20;
  // 10 is same number as used in parsesResponseHeaders() by default
  const int defaultRem = 10;
  QTest::newRow("negative-reset") << relative << u"limit: 20\nremaining: 0\nreset: -1"_s << defaultRem;
  QTest::newRow("missing-reset") << relative << u"limit: 20\nremaining: 0"_s << defaultRem;
  QTest::newRow("bad-number") << relative << u"limit: 20\nremaining: invalid\nreset: 60"_s << defaultRem;
  QTest::newRow("out-of-range") << relative << u"limit: 20\nremaining: 21\nreset: 60"_s << defaultRem;
  QTest::newRow("overflow") << relative
    << u"limit: 20\nremaining: 0\nreset: 9223372036854775807"_s << defaultRem;
  QTest::newRow("final-block") << relative
    << u"HTTP/1.1 302 Found\nlimit: 20\nremaining: 0\nreset: 60\n\nHTTP/1.1 200 OK\n"_s << defaultRem;
}

void RateLimiterTest::honorsRetryAfter() {
  QFETCH(QString, value);
  QFETCH(int, code);
  QFETCH(bool, blocked);

  RateLimiter limiter({{u"test"_s, 10, 1h}});
  limiter.updateFromResponse(u"HTTP/1.1 429 Too Many Requests\r\nrEtRy-AfTeR: "_s + value, code);
  QCOMPARE(limiter.bucketRemaining(u"test"_s), 10); // still have 10 remaining
  QCOMPARE(!limiter.rateMessage().isEmpty(), blocked); // non-empty message when blocked
  if(blocked) {
    // Neither an ordinary response nor a shorter retry can clear the deadline.
    limiter.updateFromResponse(QString(), 200);
    limiter.updateFromResponse(u"Retry-After: 1"_s, 429);
  }
  auto job = new TestJob;
  QCOMPARE(limiter.addJob(job), !blocked);
  if(blocked) delete job; // Rejected jobs remain caller-owned.
}

void RateLimiterTest::honorsRetryAfter_data() {
  QTest::addColumn<QString>("value");
  QTest::addColumn<int>("code");
  QTest::addColumn<bool>("blocked");

  QTest::newRow("seconds") << u"60"_s << 429 << true;
  const QDateTime then = QDateTime::currentDateTimeUtc().addSecs(60);
  // absolute time requires IMF fixed date format
  QTest::newRow("http-date") << QLocale::c().toString(then, u"ddd, dd MMM yyyy HH:mm:ss 'GMT'"_s) << 429 << true;
  QTest::newRow("status-line") << u"60"_s << 0 << true;
  QTest::newRow("not-429") << u"60"_s << 200 << false;
  QTest::newRow("negative") << u"-1"_s << 429 << false;
  QTest::newRow("invalid") << u"later"_s << 429 << false;
  QTest::newRow("overflow") << u"9223372036854775807"_s << 429 << false;
  QTest::newRow("zero") << u"0"_s << 429 << false;
  QTest::newRow("past") << u"Wed, 01 Jan 2020 00:00:00 GMT"_s << 429 << false;
}

void RateLimiterTest::resumesAfterRetryAfter() {
  RateLimiter limiter({{u"test"_s, 10, 1h}});
  auto job = new TestJob;
  QVERIFY(limiter.addJob(job));
  limiter.updateFromResponse(u"Retry-After: 1"_s, 429);
  QTest::qWait(50);
  QCOMPARE(job->resumeCount(), 0); // waiting 50 msec is not long enough to finish
  QTRY_COMPARE_WITH_TIMEOUT(job->resumeCount(), 1, 2000);
  QVERIFY(limiter.rateMessage().isEmpty());
}
