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

#include "barchart.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QBarSet>
#include <QHorizontalBarSeries>
#include <QHorizontalStackedBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QCategoryAxis>
#include <QGraphicsLayout>

using Tellico::BarChart;

BarChart::BarChart(const QStringList& names_, const QList<qreal>& values_) : QChart() {
  QFont f = titleFont();
  int fontSize = f.pointSize();
  if(fontSize > 0) {
    f.setPointSize(qRound(fontSize * 1.3));
  } else {
    f.setPixelSize(qRound(f.pixelSize() * 1.3));
  }
  f.setWeight(QFont::Bold);
  setTitleFont(f);

  // horizontal bar charts
  auto series = new QHorizontalStackedBarSeries();
  QList<qreal> dummyList;
  dummyList.reserve(names_.count());
  for(int i = 0; i < names_.count(); ++i) {
    auto barSet = new QBarSet(names_.at(i));
    // to "trick" the bar chart into different colors, each new group must be in a different value position
    // so replace the previous value with 0 and insert the next one
    if(i > 0) dummyList.replace(i-1, 0);
    dummyList << values_.at(i);
    barSet->append(dummyList);
    // add the bar set and series to the chart
    series->append(barSet);
  }
  series->setLabelsVisible(true);
  series->setLabelsFormat(QStringLiteral("(@value) "));
  series->setLabelsPosition(QAbstractBarSeries::LabelsInsideEnd);
  series->setBarWidth(0.8);
  addSeries(series);

  auto axisX = new QValueAxis();
  axisX->setLabelFormat(QStringLiteral("%d"));
  addAxis(axisX, Qt::AlignBottom);
  series->attachAxis(axisX);
  if(axisX->max() < 5) axisX->setMax(5);
  axisX->applyNiceNumbers();

  auto axisY = new QBarCategoryAxis();
  axisY->append(names_);
  addAxis(axisY, Qt::AlignLeft);
  series->attachAxis(axisY);
  axisY->setGridLineVisible(false);

  layout()->setContentsMargins(0, 20, 0, 20); // default is 20, 20, 20, 20
  legend()->hide();
  setBackgroundRoundness(0);
}
