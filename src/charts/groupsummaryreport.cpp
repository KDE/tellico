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

#include "groupsummaryreport.h"
#include "barchart.h"
#include "../document.h"
#include "../entrygroup.h"
#include "../models/models.h"
#include "../models/entrygroupmodel.h"
#include "../models/groupsortmodel.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QChartView>

namespace {
  static const int MAX_GROUP_VALUES = 5;
  static const int MIN_GROUP_VALUES = 3;
  static const int NUM_CHART_COLUMNS = 2;
}

using Tellico::GroupSummaryWidget;
using Tellico::GroupSummaryReport;

GroupSummaryWidget::GroupSummaryWidget(const QString& title_, int count_, QWidget* parent_) : QScrollArea(parent_) {
  auto widget = new QWidget;
  setWidget(widget);

  m_layout = new QGridLayout(widget);
  m_layout->setContentsMargins(0, 0, 0, 0);
  m_layout->setSpacing(0);
  widget->setLayout(m_layout);

  auto titleLabel = new QLabel(i18n("Group Summary: %1", title_), widget);
  titleLabel->setAlignment(Qt::AlignCenter);
  int fontSize = qRound(QApplication::font().pointSize() * 2.0);
  titleLabel->setStyleSheet(QStringLiteral("QLabel { font-size: %1pt }").arg(QString::number(fontSize)));
  m_layout->addWidget(titleLabel, 0, 0, 1, -1);

  auto countLabel = new QLabel(i18n("Total number of entries: %1", count_), widget);
  countLabel->setAlignment(Qt::AlignCenter);
  fontSize = qRound(QApplication::font().pointSize() * 1.2);
  countLabel->setStyleSheet(QStringLiteral("QLabel { font-size: %1pt; font-style: italic }").arg(QString::number(fontSize)));
  m_layout->addWidget(countLabel, 1, 0, 1, -1);

  m_layout->addItem(new QSpacerItem(1, 10), 2, 0, 1, -1);
}

void GroupSummaryWidget::addChart(Tellico::Data::FieldPtr field_) {
  // first populate a model with the entry groups for this field
  Data::CollPtr coll = Data::Document::self()->collection();
  Data::EntryGroupDict* dict = coll->entryGroupDictByName(field_->name());
  // dict will be null for a field with no values
  if(!dict) {
    return;
  }

  EntryGroupModel* groupModel = new EntryGroupModel(this);
  groupModel->addGroups(dict->values(), QString()); // TODO: make second value optional

  GroupSortModel* model = new GroupSortModel(this);
  model->setSourceModel(groupModel);
  model->setSortRole(RowCountRole);
  model->sort(0, Qt::DescendingOrder);

  // grab the top few values in the group
  QStringList groupNames;
  QList<qreal> groupCounts;
  int numGroups = qMin(MAX_GROUP_VALUES, model->rowCount());
  for(int i = 0; i < numGroups; ++i) {
    Data::EntryGroup* group = model->data(model->index(i, 0), GroupPtrRole).value<Data::EntryGroup*>();
    if(!group || group->isEmpty()) {
      continue;
    }
    if(group->hasEmptyGroupName()) {
      ++numGroups; // bump the limit to account for the "Empty" group showing up
      continue;
    }
    if(group->fieldName() == QStringLiteral("rating")) {
      const int rating = qBound(0, group->groupName().toInt(), 10);
      groupNames << QString::fromUtf8("⭐").repeated(rating);
    } else {
      groupNames << group->groupName();
    }
    groupCounts << group->count();
  }
 // if less than minimum, we don't care, skip this field
 if(groupCounts.count() < MIN_GROUP_VALUES) {
   return;
 }

  auto chart = new BarChart(groupNames, groupCounts);
  m_charts << chart;
  chart->setTitle(i18n("Top Values: %1", field_->title()));
  chart->setTheme(qApp->palette().color(QPalette::Window).lightnessF() < 0.25 ? QChart::ChartThemeDark
                                                                              : QChart::ChartThemeLight);

  auto chartView = new QChartView(chart, widget());
  chartView->setMinimumHeight(groupNames.count() * 50);
  // avoid a transparent frame around the chart
  chartView->setBackgroundBrush(chart->backgroundBrush());
  chartView->setRenderHint(QPainter::Antialiasing);

  // start with the 3 rows for the top labels and spacer
  const int row = 3 + (m_charts.count()-1) / NUM_CHART_COLUMNS;
  const int col = (m_charts.count()-1) % NUM_CHART_COLUMNS;
  m_layout->addWidget(chartView, row, col);

  // first time through, update the background color of the whole element
  if(m_charts.count() == 1) {
    QColor bg = chart->backgroundBrush().color();
    if(!bg.isValid()) {
      auto grad = chart->backgroundBrush().gradient();
      if(grad) {
        const auto stops = grad->stops();
        bg = stops.first().second; // stops is a vector of QPair with second element the color
      }
    }
    if(bg.isValid()) {
      widget()->setStyleSheet(QStringLiteral("background-color:%1").arg(bg.name(QColor::HexArgb)));
    }
  }
}

void GroupSummaryWidget::updateGeometry() {
  static bool updating = false;
  if(updating || m_charts.isEmpty()) {
    return;
  }
  updating = true;
  for(int col = 0; col < NUM_CHART_COLUMNS; ++col) {
    updateChartColumn(col);
  }
  updating = false;
}

void GroupSummaryWidget::updateChartColumn(int column_) {
  BarChart* bestChart = nullptr;
  QRectF bestRect;
  for(int idx = column_; idx < m_charts.count(); idx += NUM_CHART_COLUMNS) {
    auto chart = m_charts.at(idx);
    if(!bestChart || chart->plotArea().left() > bestRect.left()) {
      bestChart = chart;
      bestRect = chart->plotArea();
      chart->setMargins(QMargins(20, 0, 20, 0)); // reset margins to the basic default values
    }
  }
  if(!bestChart) {
    return;
  }

  for(int idx = column_; idx < m_charts.count(); idx += NUM_CHART_COLUMNS) {
    auto chart = m_charts.at(idx);
    if(bestChart != chart) {
      const int left =  chart->margins().left()  + bestRect.left()  - chart->plotArea().left();
      const int right = chart->margins().right() - bestRect.right() + chart->plotArea().right();
      chart->setMargins(QMargins(left, 0, right, 0));
    }
  }
}

GroupSummaryReport::GroupSummaryReport() : ChartReport() {
}

QString GroupSummaryReport::title() const {
  return i18n("Group Summary Report");
}

QWidget* GroupSummaryReport::createWidget() {
  // create a new widget every time
  Data::CollPtr coll = Data::Document::self()->collection();
  Q_ASSERT(coll);
  auto widget = new GroupSummaryWidget(coll->title(), coll->entryCount());

  foreach(Data::FieldPtr field, coll->fields()) {
    if(field && field->hasFlag(Data::Field::AllowGrouped)) {
      widget->addChart(field);
    }
  }
  widget->setVisible(true);
  widget->setWidgetResizable(true);
  QTimer::singleShot(0, widget, &GroupSummaryWidget::updateGeometry);
  return widget;
}
