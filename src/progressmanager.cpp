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
ProgressManager* ProgressManager::s_self = 0;

template<typename T>
inline
uint qHash(const QPointer<T>& pointer_) {
  return qHash(pointer_.data());
}

ProgressItem::Done::~Done() {
  ProgressManager::self()->setDone(m_object);
}

ProgressItem::ProgressItem(const QString& label_, bool canCancel_)
    : m_label(label_)
    , m_canCancel(canCancel_)
    , m_progress(0)
    , m_total(0)
    , m_cancelled(false) {
}

ProgressItem::~ProgressItem() {
//  myDebug() << "~ProgressItem() - " << m_label << endl;
}

void ProgressItem::setProgress(qulonglong steps_) {
  m_progress = steps_;
  emit signalProgress(this);

  if(m_progress >= m_total) {
    setDone();
  }
}

void ProgressItem::setTotalSteps(qulonglong steps_) {
  m_total = steps_;
  emit signalTotalSteps(this);
}

void ProgressItem::setDone() {
  if(!m_cancelled) {
    m_progress = m_total;
  }
  emit signalDone(this);
  // make sure the deleting doesn't interfere with anything
  QTimer::singleShot(3000, this, SLOT(deleteLater()));
}

void ProgressItem::cancel() {
//  myDebug() << "ProgressItem::cancel()" << endl;
  if(!m_canCancel || m_cancelled) {
    return;
  }

  m_cancelled = true;
  emit signalCancelled(this);
}

ProgressManager::ProgressManager() : QObject() {
}

void ProgressManager::setProgress(QObject* owner_, qulonglong steps_) {
  if(!m_items.contains(owner_)) {
    return;
  }

  m_items[owner_] ->setProgress(steps_);
//  slotUpdateTotalProgress(); // called in ProgressItem::setProgress()
//  emit signalItemProgress(m_items[owner_]);
}

void ProgressManager::setTotalSteps(QObject* owner_, qulonglong steps_) {
  if(!m_items.contains(owner_)) {
    return;
  }

  m_items[owner_]->setTotalSteps(steps_);
//  updateTotalProgress(); // called in ProgressItem::setTotalSteps()
}

void ProgressManager::setDone(QObject* owner_) {
  if(!m_items.contains(owner_)) {
    return;
  }
  setDone(m_items[owner_]);
}

void ProgressManager::setDone(ProgressItem* item_) {
  if(!item_) {
    myDebug() << "ProgressManager::setDone() - null ProgressItem!" << endl;
    return;
  }
  item_->setDone();
//  updateTotalProgress();
}

void ProgressManager::slotItemDone(ProgressItem* item_) {
// cancel ends up removing it from the map, so make a copy
  ProgressMap map = m_items;
  for(ProgressMap::Iterator it = map.begin(); it != map.end(); ++it) {
    if(static_cast<ProgressItem*>(it.value()) == item_) {
      m_items.remove(it.key());
      break;
    }
  }
  slotUpdateTotalProgress();
//  emit signalItemDone(item_);
}

ProgressItem& ProgressManager::newProgressItemImpl(QObject* owner_,
                                                   const QString& label_,
                                                   bool canCancel_) {
//  myDebug() << "ProgressManager::newProgressItem() - " << owner_->className() << ":" << label_ << endl;
  if(m_items.contains(owner_)) {
    return *m_items[owner_];
  }

  ProgressItem* item = new ProgressItem(label_, canCancel_);
  m_items.insert(owner_, item);

  connect(item, SIGNAL(signalTotalSteps(ProgressItem*)), SLOT(slotUpdateTotalProgress()));
  connect(item, SIGNAL(signalProgress(ProgressItem*)),   SLOT(slotUpdateTotalProgress()));
  connect(item, SIGNAL(signalDone(ProgressItem*)),       SLOT(slotUpdateTotalProgress()));
  connect(item, SIGNAL(signalDone(ProgressItem*)),       SLOT(slotItemDone(ProgressItem*)));

//  connect(item, SIGNAL(signalProgress(ProgressItem*)), SIGNAL(signalItemProgress(ProgressItem*)));
//  emit signalItemAdded(item);
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
    emit signalTotalProgress(100);
    return;
  }

  emit signalTotalProgress(100*progress/total);
}

void ProgressManager::slotCancelAll() {
// cancel ends up removing it from the map, so make a copy
  ProgressMap map = m_items;
  for(ProgressMap::ConstIterator it = map.constBegin(), end = map.constEnd(); it != end; ++it) {
    if(it.value()) {
      it.value()->cancel();
      setDone(it.value());
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

#include "progressmanager.moc"
