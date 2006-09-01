/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "progressmanager.h"
#include "tellico_debug.h"

#include <qtimer.h>

using Tellico::ProgressItem;
using Tellico::ProgressManager;
ProgressManager* ProgressManager::s_self = 0;

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

void ProgressItem::setProgress(uint steps) {
  m_progress = steps;
  emit signalProgress(this);

  if(m_progress >= m_total) {
    setDone();
  }
}

void ProgressItem::setTotalSteps(uint steps) {
  m_total = steps;
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

void ProgressManager::setProgress(const QObject* owner_, uint steps_) {
  if(!m_items.contains(owner_)) {
    return;
  }

  m_items[owner_] ->setProgress(steps_);
  updateTotalProgress();
  emit signalItemProgress(m_items[owner_]);
}

void ProgressManager::setTotalSteps(const QObject* owner_, uint steps_) {
  if(!m_items.contains(owner_)) {
    return;
  }

  m_items[owner_] ->setTotalSteps(steps_);
  updateTotalProgress();
}

void ProgressManager::setDone(const QObject* owner_) {
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
  updateTotalProgress();
}

void ProgressManager::slotItemDone(ProgressItem* item_) {
// cancel ends up removing it from the map, so make a copy
  ProgressMap map = m_items;
  for(ProgressMap::Iterator it = map.begin(); it != map.end(); ++it) {
    if(it.data() == item_) {
      m_items.remove(it.key());
      break;
    }
  }
  updateTotalProgress();
  emit signalItemDone(item_);
}

ProgressItem& ProgressManager::newProgressItemImpl(const QObject* owner_,
                                                   const QString& label_,
                                                   bool canCancel_) {
//  myDebug() << "ProgressManager::newProgressItem() - " << owner_->className() << ":" << label_ << endl;
  if(m_items.contains(owner_)) {
    return *m_items[owner_];
  }

  ProgressItem* item = new ProgressItem(label_, canCancel_);
  m_items.insert(owner_, item);

  connect(item, SIGNAL(signalProgress(ProgressItem*)), SIGNAL(signalItemProgress(ProgressItem*)));
  connect(item, SIGNAL(signalDone(ProgressItem*)), SLOT(slotItemDone(ProgressItem*)));

  emit signalItemAdded(item);
  return *item;
}

void ProgressManager::updateTotalProgress() {
  uint progress = 0;
  uint total = 0;

  for(ProgressMap::ConstIterator it = m_items.begin(); it != m_items.end(); ++it) {
    if(*it) {
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
  for(ProgressMap::ConstIterator it = map.begin(), end = map.end(); it != end; ++it) {
    it.data()->cancel();
    setDone(it.data());
  }
}

bool ProgressManager::anyCanBeCancelled() const {
  for(ProgressMap::ConstIterator it = m_items.begin(), end = m_items.end(); it != end; ++it) {
    if((*it)->canCancel()) {
      return true;
    }
  }
  return false;
}

#include "progressmanager.moc"
