/***************************************************************************
    Copyright (C) 2010 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_STATISTICSDIALOG_H
#define TELLICO_STATISTICSDIALOG_H

#include <kdialog.h>

class KPlotWidget;

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class StatisticsDialog : public KDialog {
Q_OBJECT

public:
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   */
  StatisticsDialog(const QString& groupField, QWidget* parent);
  virtual ~StatisticsDialog();

public slots:

private slots:

private:
  struct BarValue {
    BarValue(const QString& l, int c) : label(l), count(c) {}
    bool operator<(const BarValue& r) const {
      return count < r.count;
    }
    QString label;
    int count;
  };

  KPlotWidget* m_plotWidget;
};

} // end namespace
#endif
