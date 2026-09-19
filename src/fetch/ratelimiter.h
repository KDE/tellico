/***************************************************************************
 *    Copyright (C) 2026 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCH_RATELIMITER_H
#define TELLICO_FETCH_RATELIMITER_H

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QTimer>

#include <chrono>

namespace KIO {
  class Job;
}

namespace Tellico {
  namespace Fetch {

/**
 * The RateLimiter class queues KIO jobs and starts them without exceeding the rate of
 * any of the limit tiers. Each tier is a rolling window; a job consumes one unit
 * from every tier when it is resumed. A server response may replace that estimate
 * until the server's window resets.
 *
 * The limiter takes ownership of jobs successfully added to it. Jobs must be
 * configured and connected to their result handlers before being added.
 *
 * @author Robby Stephenson
 */
class RateLimiter : public QObject {
Q_OBJECT

public:
  struct Tier {
    QString name;
    int limit;
    std::chrono::milliseconds interval;
  };

  explicit RateLimiter(const QList<Tier>& tiers,
                       int maxConcurrentJobs = 1,
                       QObject* parent = nullptr);

  /**
   * Suspends and queues a job. Returns false without taking ownership if the
   * job is null, is already managed by this limiter, or cannot be suspended.
   */
  bool addJob(KIO::Job* job);

  /**
   * Queues a job and runs a nested event loop until it finishes. This is the
   * rate-limited equivalent of KJob::exec(). The job remains valid until the
   * caller returns to the event loop, allowing its result data to be read.
   */
  bool execJob(KIO::Job* job);

  /**
   * Replaces an allocation's local estimate with an authoritative snapshot
   * from the server. The allocation index is its position in the constructor
   * list. The reset time must be an absolute time supplied by the API.
   */
  bool updateBucket(const QString& tierName,
                    int limit,
                    int remaining,
                    const QDateTime& resetTime);

  int bucketRemaining(const QString& tierName) const;
  QString rateMessage(const QString& tierName) const;

private:
  struct Bucket {
    int limit;
    qint64 interval; // milliseconds of the tier window
    QList<qint64> starts; // timestamp of api calls
    int serverRemaining{-1};
    qint64 serverResetDeadline{-1};
    qint64 serverResetTime{-1};
  };

  void dispatch();
  void forgetJob(KIO::Job* job);
  void scheduleDispatch(qint64 delay = 0);
  qint64 delayUntilNextJob(qint64 now);
  void refreshBucket(Bucket& bucket, qint64 now);

  QHash<QString, Bucket> m_buckets;
  QList<QPointer<KIO::Job> > m_queue;
  QSet<KIO::Job*> m_activeJobs;
  QElapsedTimer m_clock;
  QTimer m_timer;
  int m_maxConcurrentJobs;
  bool m_valid;
};

  } // end namespace
} // end namespace

#endif
