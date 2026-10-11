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
#include "../core/tellico_strings.h"
#include "../tellico_debug.h"

#include <KIO/SimpleJob>
#include <KLocalizedString>

#include <QEventLoop>
#include <QUrl>
#include <QThread>
#include <QDateTime>
#include <QTimeZone>
#include <QMetaMethod>

#include <algorithm>

namespace {
  // max wait is 10 seconds
  static const qint64 LIMITER_MAX_WAIT = 10 * 1000;
}

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
    m_responseHeaders.insert(tier.name, tier.headers);
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

  if(delayUntilNextJob(m_clock.elapsed()) > LIMITER_MAX_WAIT) {
    return false;
  }

  // Suspend the job or avoid taking ownership
  if(!job_->suspend()) {
    return false;
  }

  job_->addMetaData(QStringLiteral("PropagateHttpHeader"), QStringLiteral("true"));
  job_->setParent(this);
  m_queue.append(job_);
  // KJob emits finished before result. Release the active slot at that point
  // so a result handler may safely submit a synchronous follow-up job. Also
  // parse the response headers for updates from the server
  connect(job_, &KJob::finished, this, [this, job_]() {
    updateFromResponse(job_->queryMetaData(QStringLiteral("HTTP-Headers")),
                       job_->queryMetaData(QStringLiteral("responsecode")).toInt());

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

void RateLimiter::updateFromResponse(const QString& rawHeaders_, int responseCode_) {
  if(QThread::currentThread() != thread()) return;

  QHash<QString, QString> headers;
  int headerCode = 0;
  const auto lines = rawHeaders_.split(QLatin1Char('\n'));
  for(const auto& line : lines) {
    // Redirects and informational responses may precede the final header block.
    if(line.startsWith(QLatin1StringView("HTTP/"), Qt::CaseInsensitive)) {
      headerCode = line.simplified().section(QLatin1Char(' '), 1, 1).toInt();
      headers.clear();
      continue;
    }
    const auto colon = line.indexOf(QLatin1Char(':'));
    if(colon < 1) continue;
    const auto name = line.left(colon).trimmed().toLower();
    const auto value = line.mid(colon + 1).trimmed();
    auto it = headers.find(name);
    if(it == headers.end()) {
      headers.insert(name, value);
    } else {
      *it += QLatin1StringView(", ") + value;
    }
  }

  const auto now = QDateTime::currentDateTimeUtc();
  for(auto it = m_responseHeaders.cbegin(); it != m_responseHeaders.cend(); ++it) {
    const auto& mapping = it.value();
    // skip if we don't know which header to look for
    if(mapping.limit.isEmpty() || mapping.remaining.isEmpty()) continue;

    bool limitOk = false, remainingOk = false, resetOk = false;
    const int limit = headers.value(mapping.limit.toLower()).toInt(&limitOk);
    const int remaining = headers.value(mapping.remaining.toLower()).toInt(&remainingOk);
    QDateTime resetTime;
    if(mapping.resetFormat == ResetFormat::FixedDelay) {
      resetOk = mapping.fixedDelay.count() > 0;
      resetTime = now.addSecs(mapping.fixedDelay.count());
    } else {
      const auto reset = headers.value(mapping.reset.toLower()).toLongLong(&resetOk);
      // reset could either be a delay or an absolute time
      resetOk = resetOk && reset >= 0 && reset <= std::numeric_limits<qint64>::max() / 1000;
      if(resetOk) {
        resetTime = mapping.resetFormat == ResetFormat::UnixSeconds ?
                    QDateTime::fromSecsSinceEpoch(reset, QTimeZone::UTC) : now.addSecs(reset);
      }
    }
    if(limitOk && remainingOk && resetOk && updateBucket(it.key(), limit, remaining, resetTime)) {
      myLog() << "RateLimiter:" << it.key() << "- remaining:" << remaining << "- reset:" << resetTime.toString(Qt::ISODate);
    }
  }

  // if response is not a 429, then we're done
  if((responseCode_ > 0 ? responseCode_ : headerCode) != 429) return;

  const auto value = headers.value(QStringLiteral("retry-after"));
  bool ok = false;
  const auto seconds = value.toLongLong(&ok);
  QDateTime resetTime;
  if(ok) {
    if(seconds < 0 || seconds > std::numeric_limits<qint64>::max() / 1000) return;
    resetTime = now.addSecs(seconds);
  } else {
    // Qt's RFC2822 parser expects a numeric offset rather than HTTP's GMT.
    QString dateValue = value;
    if(dateValue.endsWith(QLatin1StringView(" GMT"))) {
      dateValue.chop(3);
      dateValue += QLatin1StringView("+0000");
    }
    resetTime = QDateTime::fromString(dateValue, Qt::RFC2822Date).toUTC();
  }
  // if the parsing didn't work, we're done
  if(!resetTime.isValid()) return;

  const auto delay = now.msecsTo(resetTime);
  const auto elapsed = m_clock.elapsed();
  if(delay <= 0 || delay > std::numeric_limits<qint64>::max() - elapsed) return;

  // Retry-After does not identify a tier. Preserve the quota snapshots and
  // apply a separate deadline, which later responses must not shorten.
  const auto deadline = elapsed + delay;
  if(deadline > m_retryDeadline) {
    m_retryDeadline = deadline;
    m_retryTime = resetTime.toMSecsSinceEpoch();
  }
  scheduleDispatch();
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

int RateLimiter::bucketRemaining(const QString& tierName_) const {
  if(!m_buckets.contains(tierName_)) return -1;
  const auto& bucket = m_buckets[tierName_];
  int rem = bucket.serverRemaining;
  if(rem == -1) rem = bucket.limit - bucket.starts.size();
  return rem < 0 ? 0 : rem;
}

QString RateLimiter::rateMessage() const {
  const auto now = m_clock.elapsed();
  qint64 resetTime = m_retryDeadline > now ? m_retryTime : -1;
  for(const auto& bucket : m_buckets) {
    if(bucket.serverRemaining == 0 && bucket.serverResetDeadline > now) {
      resetTime = std::max(resetTime, bucket.serverResetTime);
    }
  }
  if(resetTime < 0) return QString();

  const auto reset = QDateTime::fromMSecsSinceEpoch(resetTime, QTimeZone::UTC);
  const auto minLeft = QDateTime::currentDateTimeUtc().secsTo(reset) / 60 + 1;
  return minLeft < 60 ?
    TC_I18N2(Tellico::rateLimitedReset, minLeft) : // anything less than an hour
    TC_I18N1(Tellico::rateLimited);
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
  const qint64 maximumDelay = LIMITER_MAX_WAIT;
  const int delay = static_cast<int>(std::min(delay_, maximumDelay));
  if(!m_timer.isActive() || delay < m_timer.remainingTime()) {
    m_timer.start(delay);
  }
}

qint64 RateLimiter::delayUntilNextJob(qint64 now_) {
  qint64 delay = std::max(qint64(0), m_retryDeadline - now_);
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

void RateLimiter::retryAfter(const QStringList& headers_) {
  for(const QString& header : headers_) {
    const qsizetype index = header.indexOf(QLatin1Char(':'));
    if(index < 1 || header.first(index).trimmed().compare(QLatin1StringView("retry-after"),
                                                          Qt::CaseInsensitive) != 0) {
      continue;
    }

    const QString value = header.sliced(index + 1).trimmed();
    const QDateTime now = QDateTime::currentDateTimeUtc();
    bool ok = false;
    const qint64 retrySeconds = value.toLongLong(&ok);
    const QDateTime resetTime = ok && retrySeconds >= 0 ?
                                  now.addSecs(retrySeconds) :
                                  QDateTime::fromString(value, Qt::RFC2822Date).toUTC();
    const qint64 retryDelay = now.msecsTo(resetTime);
    if(!resetTime.isValid() || retryDelay <= 0) {
      break;
    }

    // Retry-After does not identify the exhausted tier. Use the shortest
    // tier whose window contains the requested delay. If the server asks
    // for a delay beyond every configured window, use the longest tier.
    auto selected = m_buckets.end();
    auto longest = m_buckets.end();
    for(auto it = m_buckets.begin(); it != m_buckets.end(); ++it) {
      if(longest == m_buckets.end() || it->interval > longest->interval) {
        longest = it;
      }
      if(it->interval >= retryDelay &&
         (selected == m_buckets.end() || it->interval < selected->interval)) {
        selected = it;
      }
    }
    if(selected == m_buckets.end()) {
      selected = longest;
    }
    if(selected != m_buckets.end()) {
      myLog() << "Updating bucket" << selected.key() << "to" << resetTime;
      updateBucket(selected.key(), selected->limit, 0, resetTime);
    }
    break;
  }
}
