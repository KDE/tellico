/***************************************************************************
    Copyright (C) 20010 Robby Stephenson <robby@periapsis.org>
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

#include "statisticsdialog.h"
#include "../document.h"
#include "../collection.h"
#include "../entrygroup.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <KConfig>
#include <KPlotWidget>
#include <KPlotObject>
#include <KPlotAxis>

#include <QLabel>
#include <QVBoxLayout>

namespace {
  static const int STATISTICS_MIN_WIDTH = 600;
  static const int STATISTICS_MIN_HEIGHT = 420;
}

using Tellico::StatisticsDialog;

StatisticsDialog::StatisticsDialog(const QString& groupField_, QWidget* parent_)
    : KDialog(parent_) {
  setModal(false);
  setCaption(i18n("Collection Statistics"));
  setButtons(Close);
  setDefaultButton(Close);

  QWidget* mainWidget = new QWidget(this);
  setMainWidget(mainWidget);
  QVBoxLayout* topLayout = new QVBoxLayout(mainWidget);

  m_plotWidget = new KPlotWidget(mainWidget);
  topLayout->addWidget(m_plotWidget);

  m_plotWidget->axis(KPlotWidget::LeftAxis)->setLabel(QLatin1String("Count"));
  m_plotWidget->axis(KPlotWidget::BottomAxis)->setLabel(groupField_);
  m_plotWidget->axis(KPlotWidget::BottomAxis)->setTickLabelsShown(false);

  Data::CollPtr coll = Data::Document::self()->collection();
  Data::EntryGroupDict* dict = coll->entryGroupDictByName(groupField_);

  QList<BarValue> values;
  for(Data::EntryGroupDict::const_iterator i = dict->constBegin(); i != dict->constEnd(); ++i) {
    values.append(BarValue(i.value()->groupName(), i.value()->size()));
  }
  qSort(values.begin(), values.end(), qGreater<BarValue>());

  int x = 0;
  int maxy = 0;
  for(int i = 0; i < qMin(10, values.size()); ++i) {
    const BarValue& v = values.at(i);
    KPlotObject* po = new KPlotObject(Qt::red, KPlotObject::Bars);
    po->setBarBrush(Qt::red);
    po->addPoint(QPointF(++x, v.count), v.label, 0.9);
    m_plotWidget->addPlotObject(po);
    maxy = qMax(maxy, v.count);
  }

  m_plotWidget->setLimits(0.0, x+1, 0.0, 1.2*maxy);

  setMinimumWidth(qMax(minimumWidth(), STATISTICS_MIN_WIDTH));
  setMinimumHeight(qMax(minimumHeight(), STATISTICS_MIN_HEIGHT));

  KConfigGroup config(KGlobal::config(), QLatin1String("Statistics Dialog Options"));
  restoreDialogSize(config);
}

StatisticsDialog::~StatisticsDialog() {
  KConfigGroup config(KGlobal::config(), QLatin1String("Statistics Dialog Options"));
  saveDialogSize(config);
}

#include "statisticsdialog.moc"
