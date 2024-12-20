/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_GROUPSUMMARYREPORT_H
#define TELLICO_GROUPSUMMARYREPORT_H

#include "chartreport.h"
#include "../datavectors.h"

#include <QScrollArea>

class QGridLayout;

namespace Tellico {

class BarChart;

class GroupSummaryWidget : public QScrollArea {
Q_OBJECT

public:
  GroupSummaryWidget(const QString& title, int count, QWidget* parent = nullptr);

  void addChart(Data::FieldPtr field);

public Q_SLOTS:
  void updateGeometry();

private:
  void updateChartColumn(int column_);

  QGridLayout* m_layout;
  QList<BarChart*> m_charts;
};

/**
 * The GroupSummary Report widget shows one or more charts as a report.
 *
 * @author Robby Stephenson
 */
class GroupSummaryReport : public ChartReport {

public:
  GroupSummaryReport();

  virtual QString title() const override;
  virtual QWidget* createWidget() override;
};

} // end namespace
#endif
