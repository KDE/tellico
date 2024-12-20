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

#ifndef TELLICO_GUI_DOCKWIDGET_H
#define TELLICO_GUI_DOCKWIDGET_H

#include <QDockWidget>

namespace Tellico {
  namespace GUI {

class DockTitleBar : public QWidget {
Q_OBJECT

public:
  explicit DockTitleBar(QWidget* parent_) : QWidget(parent_) {}
  ~DockTitleBar() override {};
};

class DockWidget : public QDockWidget {
Q_OBJECT

public:
  explicit DockWidget(const QString& title, QWidget* parent);
  ~DockWidget() override {};

  /**
    * @param lock If \a lock is true, the title bar of the dock-widget will get hidden so
    *             that it is not possible for the user anymore to move or undock the dock-widget.
    */
  void setLocked(bool lock);
  bool isLocked() const { return m_isLocked; }

private:
  bool m_isLocked;
  QWidget* m_dockTitleBar;
};

  } // end namespace
} //end namespace

#endif
