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

// much of this code is adapted from libkdepim
// which is GPL licensed, Copyright (c) 2004 Till Adam

#ifndef TELLICO_PROGRESSMANAGER_H
#define TELLICO_PROGRESSMANAGER_H

#include <QObject>
#include <QHash>
#include <QPointer>

namespace Tellico {

class ProgressManager;

/**
 * @author Robby Stephenson
 */
class ProgressItem : public QObject {
Q_OBJECT

friend class ProgressManager;

public:
  class Done {
  public:
    Done(QObject* object);
    ~Done();
  private:
    Q_DISABLE_COPY(Done)
    QPointer<QObject> m_object;
  };

  bool canCancel() const { return m_canCancel; }
  const QString& label() const { return m_label; }
  void setLabel(const QString& label);

  qulonglong progress() const { return m_progress; }
  void setProgress(qulonglong steps);
  qulonglong totalSteps() const { return m_total; }
  void setTotalSteps(qulonglong steps);
  void setDone();

  void cancel();

Q_SIGNALS:
  void signalProgress(Tellico::ProgressItem* item);
  void signalDone(Tellico::ProgressItem* item);
  void signalCancelled(Tellico::ProgressItem* item);
  void signalTotalSteps(Tellico::ProgressItem* item);

protected:
  /* Only to be used by the ProgressManager */
  ProgressItem(const QString& label, bool canCancel);
  virtual ~ProgressItem();

private:
  QString m_label;
  bool m_canCancel;
  qulonglong m_progress;
  qulonglong m_total;
  bool m_cancelled;
  bool m_done;
};

/**
 * @author Robby Stephenson
 */
class ProgressManager : public QObject {
Q_OBJECT

public:
  virtual ~ProgressManager();

  static ProgressManager* self() { if(!s_self) s_self = new ProgressManager(); return s_self; }

  ProgressItem& newProgressItem(QObject* owner, const QString& label, bool canCancel = false) {
    return newProgressItemImpl(owner, label, canCancel);
  }

  bool anyCanBeCancelled() const;

Q_SIGNALS:
//  void signalItemAdded(ProgressItem* item);
//  void signalItemProgress(ProgressItem* item);
//  void signalItemDone(ProgressItem* item);
//  void signalItemCancelled(ProgressItem* item);
  void signalTotalProgress(qulonglong progress);

public Q_SLOTS:
  void slotCancelAll();
  void setProgress(QObject* owner, qulonglong steps);
  void setTotalSteps(QObject* owner, qulonglong steps);
  void setDone(QObject* owner);

private Q_SLOTS:
  void slotItemDone(Tellico::ProgressItem* item);
  void slotUpdateTotalProgress();

private:
  ProgressManager();
  ProgressManager(const ProgressManager&); // no copies

  ProgressItem& newProgressItemImpl(QObject* owner, const QString& label, bool canCancel);
  void setDone(ProgressItem* item);

  typedef QHash<QPointer<QObject>, QPointer<ProgressItem> > ProgressMap;
  ProgressMap m_items;

  static ProgressManager* s_self;
};

} // end namespace

#endif
