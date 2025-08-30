/***************************************************************************
    Copyright (C) 2002-2009 Robby Stephenson <robby@periapsis.org>
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

#include "tabwidget.h"

#include <KStandardAction>

#include <QTabBar>

using Tellico::GUI::TabWidget;

TabWidget::TabWidget(QWidget* parent_)
    : QTabWidget(parent_) {
  auto action = KStandardAction::next(this, &TabWidget::slotNextTab, this);
  action->setShortcuts(KStandardShortcut::tabNext());
  addAction(action);

  action = KStandardAction::prior(this, &TabWidget::slotPrevTab, this);
  action->setShortcuts(KStandardShortcut::tabPrev());
  addAction(action);
}

void TabWidget::setFocusToFirstChild() {
  QWidget* page = currentWidget();
  Q_ASSERT(page);
  QList<QWidget*> list = page->findChildren<QWidget*>();
  foreach(QWidget* w, list) {
    if(!w->isHidden() && w->focusPolicy() != Qt::NoFocus) {
      w->setFocus();
      break;
    }
  }
}

void TabWidget::setTabBarHidden(bool hide_) {
  QTabBar* tabBar = findChild<QTabBar *>();
  Q_ASSERT(tabBar);
  tabBar->setHidden(hide_);
}

void TabWidget::slotNextTab() {
  const auto next = currentIndex() + 1;
  setCurrentIndex(next >= count() ? 0 : next);
}

void TabWidget::slotPrevTab() {
  const auto prev = currentIndex() - 1;
  setCurrentIndex(prev < 0 ? count() - 1 : prev);
}
