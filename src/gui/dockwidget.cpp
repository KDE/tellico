/***************************************************************************
    Copyright (C) 2019 Robby Stephenson <robby@periapsis.org>
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

#include "dockwidget.h"

using Tellico::GUI::DockWidget;

DockWidget::DockWidget(const QString& title_, QWidget* parent_)
  : QDockWidget(title_, parent_)
  , m_isLocked(false)
  , m_dockTitleBar(nullptr) {
  setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
}

void DockWidget::setLocked(bool lock_) {
  if(lock_ == m_isLocked) {
    return;
  }
  m_isLocked = lock_;

  if(lock_) {
    if(!m_dockTitleBar) {
      m_dockTitleBar = new DockTitleBar(this);
    }
    setTitleBarWidget(m_dockTitleBar);
    setFeatures(QDockWidget::NoDockWidgetFeatures);
  } else {
    setTitleBarWidget(nullptr);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
  }
}
