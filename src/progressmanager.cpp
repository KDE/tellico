/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "progressmanager.h"
#include "tellico_debug.h"

#include <QTimer>

using Tellico::ProgressItem;
using Tellico::ProgressManager;
ProgressManager* ProgressManager::s_self = nullptr;

template<typename T>
inline
size_t qHash(const QPointer<T>& pointer_) {
  return qHash(pointer_.data());
}

ProgressItem::Done::Done(QObject* obj) : m_object(obj) {
//  myDebug() << "**Done Item for" << m_object->metaObject()->className();
}

ProgressItem::Done::~Done() {
  // m_object might have been deleted, so check for existence
  if(m_object) {
    ProgressManager::self()->setDone(m_object);
  }
}

/* *********************************************** */

ProgressItem::ProgressItem(const QString& label_, bool canCancel_)
    : m_label(label_)
    , m_canCancel(canCancel_)
    , m_progress(0)
    , m_total(0)
    , m_cancelled(false)
    , m_done(false) {
}

ProgressItem::~ProgressItem() {
}

void ProgressItem::setProgress(qulonglong steps_) {
  if(m_done) {
    myDebug() << "Setting progress on item already done";
    return;
  }
  m_progress = steps_;
  Q_EMIT signalProgress(this);

  if(m_progress >= m_total) {
    setDone();
  }
}

void ProgressItem::setTotalSteps(qulonglong steps_) {
  m_total = steps_;
  Q_EMIT signalTotalSteps(this);
}

void ProgressItem::setDone() {
  if(!m_cancelled) {
    m_progress = m_total;
  }
  if(m_done) {
    myDebug() << "ProgressItem::setDone() - Progress item is already done";
  } else {
    m_done = true;
    Q_EMIT signalDone(this);
  }
  // make sure the deleting doesn't interfere with anything
  QTimer::singleShot(3000, this, &QObject::deleteLater);
}

void ProgressItem::cancel() {
  if(!m_canCancel || m_cancelled) {
    return;
  }

  m_cancelled = true;
  Q_EMIT signalCancelled(this);
}

/* *********************************************** */

ProgressManager::ProgressManager() : QObject() {
}

ProgressManager::~ProgressManager() {
}

void ProgressManager::setProgress(QObject* owner_, qulonglong steps_) {
  Q_ASSERT(owner_);
  if(!owner_ || !m_items.contains(owner_)) {
    return;
  }

  m_items[owner_] ->setProgress(steps_);
//  slotUpdateTotalProgress(); // called in ProgressItem::setProgress()
//  Q_EMIT signalItemProgress(m_items[owner_]);
}

void ProgressManager::setTotalSteps(QObject* owner_, qulonglong steps_) {
  Q_ASSERT(owner_);
  if(!owner_ || !m_items.contains(owner_)) {
    return;
  }

  m_items[owner_]->setTotalSteps(steps_);
//  updateTotalProgress(); // called in ProgressItem::setTotalSteps()
}

void ProgressManager::setDone(QObject* owner_) {
  Q_ASSERT(owner_);
  if(!owner_ || !m_items.contains(owner_)) {
    return;
  }
  setDone(m_items[owner_]);
}

void ProgressManager::setDone(ProgressItem* item_) {
  if(!item_) {
    myDebug() << "null ProgressItem!";
    return;
  }
  item_->setDone();
}

void ProgressManager::slotItemDone(ProgressItem* item_) {
  for(ProgressMap::Iterator it = m_items.begin(); it != m_items.end(); ++it) {
    if(it.value() == item_) {
      m_items.erase(it);
      break;
    }
  }
  slotUpdateTotalProgress();
}

ProgressItem& ProgressManager::newProgressItemImpl(QObject* owner_,
                                                   const QString& label_,
                                                   bool canCancel_) {
  Q_ASSERT(owner_);
//  myDebug() << "Progress Item for" << owner_->metaObject()->className() << ":" << label_;
  if(m_items.contains(owner_)) {
    return *m_items[owner_];
  }

  ProgressItem* item = new ProgressItem(label_, canCancel_);
  m_items.insert(owner_, item);

  connect(item, &Tellico::ProgressItem::signalTotalSteps,
          this, &Tellico::ProgressManager::slotUpdateTotalProgress);
  connect(item, &Tellico::ProgressItem::signalProgress,
          this, &Tellico::ProgressManager::slotUpdateTotalProgress);
  connect(item, &Tellico::ProgressItem::signalDone,
          this, &Tellico::ProgressManager::slotItemDone);

  return *item;
}

void ProgressManager::slotUpdateTotalProgress() {
  qulonglong progress = 0;
  qulonglong total = 0;

  for(ProgressMap::ConstIterator it = m_items.constBegin(); it != m_items.constEnd(); ++it) {
    if(it.value()) {
      progress += (*it)->progress();
      total += (*it)->totalSteps();
    }
  }

  if(total == 0) {
    if(!m_items.isEmpty()) {
      Q_EMIT signalTotalProgress(100);
    }
    return;
  }

  Q_EMIT signalTotalProgress(100*progress/total);
}

void ProgressManager::slotCancelAll() {
  for(ProgressMap::Iterator it = m_items.begin(), end = m_items.end(); it != end; ++it) {
    if(it.value()) {
      it.value()->cancel();
    }
  }
}

bool ProgressManager::anyCanBeCancelled() const {
  for(ProgressMap::ConstIterator it = m_items.constBegin(), end = m_items.constEnd(); it != end; ++it) {
    if(it.value() && it.value()->canCancel()) {
      return true;
    }
  }
  return false;
}
