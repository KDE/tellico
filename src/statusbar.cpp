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

#include "statusbar.h"
#include "progressmanager.h"
#include "tellico_debug.h"
#include "gui/progress.h"

#include <klocale.h>
#include <kapplication.h>
#include <kpushbutton.h>
#include <kiconloader.h>

#include <QObject>
#include <QPainter>
#include <QStyle>
#include <QTimer>
#include <QToolTip>
#include <QLabel>

using Tellico::StatusBar;
StatusBar* StatusBar::s_self = 0;

StatusBar::StatusBar(QWidget* parent_) : KStatusBar(parent_) {
  s_self = this;

  // don't care about text and id
  m_mainLabel = new QLabel(this);
  m_mainLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_mainLabel->setIndent(4);
  insertPermanentWidget(0, m_mainLabel, 3 /*stretch*/);

  m_countLabel = new QLabel(this);
  m_countLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_countLabel->setIndent(4);
  insertPermanentWidget(1, m_countLabel, 0);

  m_progress = new GUI::Progress(100, this);
  addPermanentWidget(m_progress, 1);

  m_cancelButton = new KPushButton(KStandardGuiItem::cancel(), this);
  m_cancelButton->setText(QString());
  m_cancelButton->setToolTip(i18n("Cancel"));
  addPermanentWidget(m_cancelButton, 0);

  m_progress->hide();
  m_cancelButton->hide();

  ProgressManager* pm = ProgressManager::self();
  connect(pm, SIGNAL(signalTotalProgress(qulonglong)), SLOT(slotProgress(qulonglong)));
  connect(m_cancelButton, SIGNAL(clicked()), pm, SLOT(slotCancelAll()));
}

void StatusBar::ensurePolished() const {
  KStatusBar::ensurePolished();

  int h = 0;
  QList<QWidget*> list = findChildren<QWidget*>();
  foreach(QWidget* o, list) {
    int _h = o->minimumSizeHint().height();
    if(_h > h) {
      h = _h;
    }
  }

  h -= 4; // hint from amarok, it's too big usually

  foreach(QObject* o, list) {
    static_cast<QWidget*>(o)->setFixedHeight(h);
  }
}

void StatusBar::clearStatus() {
  setStatus(i18n("Ready."));
}

void StatusBar::setStatus(const QString& status_) {
  // always add a space for asthetics
  m_mainLabel->setText(status_ + QLatin1Char(' '));
}

void StatusBar::setCount(const QString& count_) {
  m_countLabel->setText(count_ + QLatin1Char(' '));
}

void StatusBar::slotProgress(qulonglong progress_) {
  // yes, yes, yes, casting from longlong to int is bad, I know...
  m_progress->setValue(progress_);
  if(m_progress->isDone()) {
    m_progress->hide();
    m_cancelButton->hide();
  } else if(m_progress->isHidden()) {
    m_progress->show();
    if(ProgressManager::self()->anyCanBeCancelled()) {
      m_cancelButton->show();
    }
    kapp->processEvents(); // needed so the window gets updated ???
  }
}

void StatusBar::slotUpdate() {
/*
  myDebug() << "" << m_progress->isShown();
  if(m_progressBox->isEmpty()) {
    QTimer::singleShot(0, m_progress, SLOT(hide()));
//    m_progressBox->hide();
  } else {
    QTimer::singleShot(0, m_progress, SLOT(show()));
//    m_progressBox->show();
  }
*/
}

#include "statusbar.moc"
