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

#include "collectionsizereport.h"
#include "../document.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QDateTime>
#include <QApplication>
#include <QVBoxLayout>
#include <QChartView>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using namespace QtCharts;
#endif
using Tellico::CollectionSizeReport;

CollectionSizeReport::CollectionSizeReport() : ChartReport() {
}

QString CollectionSizeReport::title() const {
  return i18n("Collection Size");
}

QWidget* CollectionSizeReport::createWidget() {
  // create a new widget every time
  Data::CollPtr coll = Data::Document::self()->collection();
  Q_ASSERT(coll);

  const QString cdate = QStringLiteral("cdate");
  if(!coll->hasField(cdate)) {
    myDebug() << "No cdate field in collection";
    return nullptr;
  }

  int emptyCount = 0;
  QMap<QString, int> entryDateCounts;
  foreach(Data::EntryPtr entry, coll->entries()) {
    auto date = entry->field(cdate);
    if(date.isEmpty()) {
      emptyCount++;
    } else {
      entryDateCounts[date]++;
    }
  }

  auto series = new QLineSeries;
  const QStringList dates = entryDateCounts.keys(); // already sorted
  int totalCount = emptyCount; // assume empty values predate earliest date
  foreach(const QString& date, dates) {
    QDateTime momentInTime;
    momentInTime.setDate(QDate::fromString(date, Qt::ISODate));
    if(momentInTime.isValid()) {
      auto msecs = momentInTime.toMSecsSinceEpoch();
      // for a step chart, add a point just before this one with the previous total
      // but avoid huge step from initial zero to next value
      if(totalCount > 0) {
        series->append(msecs-1, totalCount);
      }
      totalCount += entryDateCounts.value(date);
      series->append(msecs, totalCount);
    }
  }
  // add current date
  QDateTime now = QDateTime::currentDateTimeUtc();
  series->append(now.toMSecsSinceEpoch(), coll->entryCount());
  series->setName(i18n("Total entries: %1", coll->entryCount()));

  auto chart = new QChart;
  chart->addSeries(series);
  chart->setTitle(i18n("Collection Size Over Time"));
  chart->setTheme(qApp->palette().color(QPalette::Window).lightnessF() < 0.25 ? QChart::ChartThemeDark
                                                                              : QChart::ChartThemeLight);
  QFont f = chart->titleFont();
  int fontSize = f.pointSize();
  if(fontSize > 0) {
    f.setPointSize(qRound(fontSize * 1.3));
  } else {
    f.setPixelSize(qRound(f.pixelSize() * 1.3));
  }
  f.setWeight(QFont::Bold);
  chart->setTitleFont(f);

  QPen pen = series->pen();
  pen.setWidth(3);
  series->setPen(pen);

  auto axisX = new QDateTimeAxis;
  axisX->setTickCount(10);
  axisX->setFormat(QStringLiteral("MMM yyyy"));
  chart->addAxis(axisX, Qt::AlignBottom);
  series->attachAxis(axisX);

  auto axisY = new QValueAxis;
  axisY->setLabelFormat(QStringLiteral("%d"));
  axisY->setTitleText(i18n("Collection Size"));
  chart->addAxis(axisY, Qt::AlignLeft);
  series->attachAxis(axisY);
  if(axisY->max() < 5) axisY->setMax(5);
  axisY->applyNiceNumbers();
  // get the lowest tick off the minimum
  if(!series->points().isEmpty() && axisY->min() == series->points().at(0).y()) {
    // add 10% padding below lowest value
    if(axisY->max() == axisY->min()) {
      axisY->setMin(0);
      axisY->setMax(1.1 * axisY->max());
    } else {
      const auto range = axisY->max() - axisY->min();
      const auto delta = qMax(0.0, 0.1 * qMin(range, axisY->max()));
      const auto newMin = axisY->min() - delta;
      if(newMin > 0.5) {
        axisY->setMin(static_cast<int>(newMin));
      }
    }
  }

  auto widget = new QWidget;
  auto layout = new QVBoxLayout(widget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  auto chartView = new QChartView(chart, widget);
  chartView->setRenderHint(QPainter::Antialiasing);
  // avoid a transparent frame around the chart
  chartView->setBackgroundBrush(chart->backgroundBrush());
  layout->addWidget(chartView);
  widget->setLayout(layout);
  return widget;
}
