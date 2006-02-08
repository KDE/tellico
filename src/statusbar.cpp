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

#include "statusbar.h"
#include "tellico_debug.h"
#include "progressmanager.h"
#include "tellico_debug.h"

#include <klocale.h>
#include <kapplication.h>
#include <kpushbutton.h>
#include <kiconloader.h>

#include <qobjectlist.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qtimer.h>
#include <qtooltip.h>

using Tellico::StatusBar;
StatusBar* StatusBar::s_self = 0;

StatusBar::StatusBar(QWidget* parent_) : KStatusBar(parent_) {
  s_self = this;

  // don't care about text and id
  m_mainLabel = new KStatusBarLabel(QString(), 0, this);
  m_mainLabel->setIndent(4);
  m_mainLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  addWidget(m_mainLabel, 3 /*stretch*/, true /*permanent*/);

  m_countLabel = new KStatusBarLabel(QString(), 1, this);
  m_countLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_countLabel->setIndent(4);
  addWidget(m_countLabel, 0, true);

  m_progress = new GUI::Progress(100, this);
  addWidget(m_progress, 1, true);
  m_cancelButton = new KPushButton(SmallIcon(QString::fromLatin1("cancel")), QString::null, this);
  QToolTip::add(m_cancelButton, i18n("Cancel"));
  addWidget(m_cancelButton, 0, true);
  m_progress->hide();
  m_cancelButton->hide();

  ProgressManager* pm = ProgressManager::self();
  connect(pm, SIGNAL(signalTotalProgress(uint)), SLOT(slotProgress(uint)));
  connect(m_cancelButton, SIGNAL(clicked()), pm, SLOT(slotCancelAll()));
}

void StatusBar::polish() {
  KStatusBar::polish();

  int h = 0;
  QObjectList* list = queryList("QWidget", 0, false, false);
  for(QObject* o = list->first(); o; o = list->next()) {
    int _h = static_cast<QWidget*>(o)->minimumSizeHint().height();
    if(_h > h) {
      h = _h;
    }
  }

  h -= 4; // hint from amarok, it's too big usually

  for(QObject* o = list->first(); o; o = list->next()) {
    static_cast<QWidget*>(o)->setFixedHeight(h);
  }

  delete list;
}

void StatusBar::clearStatus() {
  setStatus(i18n("Ready."));
}

void StatusBar::setStatus(const QString& status_) {
  // always add a space for asthetics
  m_mainLabel->setText(status_ + ' ');
}

void StatusBar::setCount(const QString& count_) {
  m_countLabel->setText(count_ + ' ');
}

void StatusBar::slotProgress(uint progress_) {
  m_progress->setProgress(progress_);
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
  myDebug() << "StatusBar::slotUpdate() - " << m_progress->isShown() << endl;
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
