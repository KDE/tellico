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

#include "ratelimiter.h"
#include "../tellico_debug.h"

#include <KIO/SimpleJob>

#include <QEventLoop>
#include <QUrl>
#include <QThread>
#include <QDateTime>
#include <QMetaMethod>

#include <algorithm>
#include <limits>

using Tellico::Fetch::RateLimiter;

RateLimiter::RateLimiter(const QList<Tier>& tiers_, int maxJobs_, QObject* parent_)
    : QObject(parent_), m_maxConcurrentJobs(maxJobs_), m_valid(true) {
  if(tiers_.isEmpty() || maxJobs_ < 1) {
    m_valid = false;
  }
  for(const auto& tier : tiers_) {
    const qint64 interval = tier.interval.count(); // milliseconds
    if(tier.limit < 1 || interval < 1 || m_buckets.contains(tier.name)) {
      m_valid = false;
      continue;
    }
    m_buckets.insert(tier.name, {tier.limit, interval, {}});
  }

  m_clock.start();
  m_timer.setSingleShot(true);
  m_timer.setTimerType(Qt::PreciseTimer);
  connect(&m_timer, &QTimer::timeout, this, &RateLimiter::dispatch);
}

bool RateLimiter::addJob(KIO::Job* job_) {
  if(!m_valid || !job_ || job_->thread() != thread() ||
     m_activeJobs.contains(job_) || m_queue.contains(job_)) {
    return false;
  }

  // Suspend the job or avoid taking ownership
  if(!job_->suspend()) {
    return false;
  }

  job_->setParent(this);
  m_queue.append(job_);
  // KJob emits finished before result. Release the active slot at that point
  // so a result handler may safely submit a synchronous follow-up job.
  connect(job_, &KJob::finished, this, [this, job_]() {
    forgetJob(job_);
  });
  connect(job_, &QObject::destroyed, this, [this, job_]() {
    forgetJob(job_);
  });
  scheduleDispatch();
  return true;
}

bool RateLimiter::execJob(KIO::Job* job_) {
  if(!job_) {
    return false;
  }

  QEventLoop loop;
  QPointer<KIO::Job> guardedJob(job_);
  connect(job_, &KJob::result, &loop, &QEventLoop::quit);
  connect(job_, &QObject::destroyed, &loop, &QEventLoop::quit);

  const bool wasAutoDelete = job_->isAutoDelete();
  job_->setAutoDelete(false);
  if(!addJob(job_)) {
    job_->setAutoDelete(wasAutoDelete);
    return false;
  }

  loop.exec(QEventLoop::ExcludeUserInputEvents);
  const bool success = guardedJob && !guardedJob->error();
  if(guardedJob && wasAutoDelete) {
    guardedJob->deleteLater();
  }
  return success;
}

bool RateLimiter::updateBucket(const QString& tierName_,
                               int limit_,
                               int remaining_,
                               const QDateTime& resetTime_) {
  if(QThread::currentThread() != thread() ||
     limit_ < 1 || remaining_ < 0 || remaining_ > limit_ ||
     !resetTime_.isValid()) {
    return false;
  }
  if(!m_buckets.contains(tierName_)) {
    myDebug() << "No tier bucket named" << tierName_;
    return false;
  }

  auto& bucket = m_buckets[tierName_];
  bucket.limit = limit_;
  bucket.starts.clear();

  const QDateTime resetTime = resetTime_.toUTC();
  const qint64 resetTimeMsec = resetTime.toMSecsSinceEpoch();
  const qint64 resetDelay = QDateTime::currentDateTimeUtc().msecsTo(resetTime);

  if(resetDelay > 0) {
    // Responses from concurrent jobs can arrive out of order. Remaining
    // calls cannot increase within one server window, and may already
    // include local requests started after the response being processed.
    if(bucket.serverResetTime == resetTimeMsec &&
       bucket.serverRemaining >= 0) {
      bucket.serverRemaining = std::min(bucket.serverRemaining, remaining_);
    } else {
      bucket.serverRemaining = remaining_;
    }

    bucket.serverResetDeadline = m_clock.elapsed() + resetDelay;
    bucket.serverResetTime = resetTimeMsec;
  } else {
    // The server window has already rolled over. Start a fresh local window
    // rather than blocking on a stale Remaining value.
    bucket.serverRemaining = -1;
    bucket.serverResetDeadline = -1;
    bucket.serverResetTime = -1;
  }

  scheduleDispatch();
  return true;
}

qsizetype RateLimiter::queuedJobCount() const {
  return m_queue.size();
}

void RateLimiter::dispatch() {
  while(m_activeJobs.count() < m_maxConcurrentJobs) {
    while(!m_queue.isEmpty() && m_queue.constFirst().isNull()) {
      m_queue.removeFirst();
    }
    if(m_queue.isEmpty()) {
      return;
    }

    const auto now = m_clock.elapsed();
    const auto delay = delayUntilNextJob(now);
    if(delay > 0) {
      scheduleDispatch(delay);
      return;
    }

    auto nextJob = m_queue.takeFirst();
    if(!nextJob) {
      continue;
    }

    for(auto& bucket : m_buckets) {
      // marks the start time for every bucket
      bucket.starts.append(now);
      if(bucket.serverRemaining >= 0) {
        --bucket.serverRemaining;
      }
    }
    m_activeJobs.insert(nextJob);
    if(!nextJob->resume()) {
      m_activeJobs.remove(nextJob);
      // remove the start times that just got added
      for(auto& bucket : m_buckets) {
        bucket.starts.removeLast();
        if(bucket.serverRemaining >= 0) {
          ++bucket.serverRemaining;
        }
      }
      nextJob->kill(KJob::EmitResult);
    }
  }
}

void RateLimiter::forgetJob(KIO::Job* job_) {
  m_activeJobs.remove(job_);
  m_queue.removeAll(job_);
  for(auto it = m_queue.begin(); it != m_queue.end();) {
    if(it->isNull()) {
      it = m_queue.erase(it);
    } else {
      ++it;
    }
  }
  scheduleDispatch();
}

void RateLimiter::scheduleDispatch(qint64 delay_) {
  const qint64 maximumDelay = std::numeric_limits<int>::max();
  const int delay = static_cast<int>(std::min(delay_, maximumDelay));
  if(!m_timer.isActive() || delay < m_timer.remainingTime()) {
    m_timer.start(delay);
  }
}

qint64 RateLimiter::delayUntilNextJob(qint64 now_) {
  qint64 delay = 0;
  for(auto& bucket : m_buckets) {
    refreshBucket(bucket, now_);
    if(bucket.serverRemaining == 0) {
      delay = std::max(delay, bucket.serverResetDeadline - now_);
    }
    while(!bucket.starts.isEmpty() &&
          now_ - bucket.starts.constFirst() >= bucket.interval) {
      bucket.starts.removeFirst();
    }
    if(bucket.starts.size() >= bucket.limit) {
      delay = std::max(delay,
                       bucket.interval - (now_ - bucket.starts.constFirst()));
    }
  }
  return delay;
}

void RateLimiter::refreshBucket(Bucket& bucket_, qint64 now_) {
  if(bucket_.serverResetDeadline >= 0 &&
     now_ >= bucket_.serverResetDeadline) {
    bucket_.starts.clear();
    bucket_.serverRemaining = -1;
    bucket_.serverResetDeadline = -1;
    bucket_.serverResetTime = -1;
  }
}