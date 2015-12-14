/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
    Copyright (C) 2011 Pedro Miguel Carvalho <kde@pmc.com.pt>
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

// much of this code is adapted from amarok
// which is GPL licensed, Copyright (C) 2005 by Max Howell

#ifndef TELLICO_STATUSBAR_H
#define TELLICO_STATUSBAR_H

#include <QStatusBar>

class QLabel;
class QPushButton;

namespace Tellico {
  namespace GUI {
    class Progress;
  }
  class MainWindow;

/**
 * @author Robby Stephenson
 */
class StatusBar : public QStatusBar {
Q_OBJECT

public:
  void clearStatus();
  void setStatus(const QString& status);
  void setCount(const QString& count);

  static StatusBar* self() { return s_self; }

  virtual void ensurePolished() const;

private slots:
  void slotProgress(qulonglong progress);
  void slotUpdate();

private:
  static StatusBar* s_self;

  friend class MainWindow;

  StatusBar(QWidget* parent);

  QLabel* m_mainLabel;
  QLabel* m_countLabel;
  GUI::Progress* m_progress;
  QPushButton* m_cancelButton;
};

}

#endif
