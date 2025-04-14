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

#include "yeardistributionreport.h"
#include "../document.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QApplication>
#include <QVBoxLayout>
#include <QChartView>
#include <QBarSet>
#include <QBarSeries>
#include <QValueAxis>
#include <QGraphicsSimpleTextItem>

using Tellico::YearDistributionReport;

class YearDistributionReport::View : public QChartView {
  Q_OBJECT
public:
  View(QChart* chart, QWidget* parent) : QChartView(chart, parent), m_tip(nullptr) {};

public:
  void tooltip(bool status, int index, QBarSet* barSet) {
    if(status) {
      if(!m_tip) m_tip = new QGraphicsSimpleTextItem(chart());
      m_tip->setText(QString::number(index) + QLatin1String(" : ") + QString::number(barSet->at(index)));
      const QPointF p(index, barSet->at(index));
      m_tip->setPos(chart()->mapToPosition(p) + QPointF(5, -20));
      m_tip->setZValue(11);
      m_tip->show();
    } else {
      m_tip->hide();
    }
  }

private:
  QGraphicsSimpleTextItem* m_tip;
};

YearDistributionReport::YearDistributionReport() : ChartReport() {
}

QString YearDistributionReport::title() const {
  return i18n("Year Distribution");
}

QWidget* YearDistributionReport::createWidget() {
  // create a new widget every time
  Data::CollPtr coll = Data::Document::self()->collection();
  Q_ASSERT(coll);

  QString yearField = QStringLiteral("year");
  if(!coll->hasField(yearField)) {
    yearField = QStringLiteral("pub_year");
  }
  if(!coll->hasField(yearField)) {
    myDebug() << "No year field in collection";
    return nullptr;
  }
  auto field = coll->fieldByName(yearField);
  Q_ASSERT(field);
  const bool multiple = field->hasFlag(Data::Field::AllowMultiple);

  QMap<QString, int> entryYearCount;
  foreach(Data::EntryPtr entry, coll->entries()) {
    const QString year = entry->field(yearField);
    if(year.isEmpty()) {
      continue;
    }
    QStringList years;
    if(multiple) {
      years = FieldFormat::splitValue(year);
    } else {
      years << year;
    }
    for(const QString& y : std::as_const(years)) {
      entryYearCount[y]++;
    }
  }

  if(entryYearCount.isEmpty()) {
    return nullptr;
  }

  bool ok;
  const uint minYear = entryYearCount.firstKey().toUInt(&ok);
  if(!ok) return nullptr;
  const uint maxYear = entryYearCount.lastKey().toUInt(&ok);
  if(!ok) return nullptr;

  // pad beginning of bar set with zeros in order to use the year as an index number
  QList<qreal> zeroes;
  zeroes.reserve(minYear);
  for(uint i = 0; i < minYear; ++i) zeroes.append(0);
  auto barSet = new QBarSet(yearField);
  barSet->append(zeroes);

  const uint yearDelta = maxYear-minYear;
  for(uint yd = 0; yd <= yearDelta; ++yd) {
    const QString yearString = QString::number(minYear + yd);
    // some years might not be represented in the map, the bar set will have 0 for them
    barSet->append(entryYearCount.value(yearString, 0));
  }

  auto series = new QBarSeries;
  series->append(barSet);

  auto chart = new QChart;
  chart->addSeries(series);
  chart->setTitle(i18n("Year Distribution"));
  chart->legend()->setVisible(false);
  chart->setAcceptHoverEvents(true);
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

  auto axisX = new QValueAxis;
  axisX->setLabelFormat(QStringLiteral("%d"));
  axisX->setRange(minYear, maxYear);
  axisX->setTitleText(field->title());
  chart->addAxis(axisX, Qt::AlignBottom);
  series->attachAxis(axisX);
  axisX->applyNiceNumbers();

  auto axisY = new QValueAxis;
  axisY->setLabelFormat(QStringLiteral("%d"));
  axisY->setTitleText(i18nc("Distinct number of items", "Count"));
  chart->addAxis(axisY, Qt::AlignLeft);
  series->attachAxis(axisY);
  if(axisY->max() < 5) axisY->setMax(5);
  axisY->applyNiceNumbers();
  if(axisY->max() >= 20) {
    axisY->setMinorTickCount(4);
  }
  axisY->setMinorGridLineVisible(true);

  auto widget = new QWidget;
  auto layout = new QVBoxLayout(widget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  auto chartView = new View(chart, widget);
  chartView->setRenderHint(QPainter::Antialiasing);
  // avoid a transparent frame around the chart
  chartView->setBackgroundBrush(chart->backgroundBrush());
  QObject::connect(series, &QBarSeries::hovered, chartView, &View::tooltip);
  layout->addWidget(chartView);
  widget->setLayout(layout);
  return widget;
}

#include "yeardistributionreport.moc"
