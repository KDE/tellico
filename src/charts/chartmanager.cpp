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

#include "chartmanager.h"
#include "collectionsizereport.h"
#include "groupsummaryreport.h"
#include "yeardistributionreport.h"

using Tellico::ChartManager;

ChartManager* ChartManager::self() {
  static ChartManager manager;
  return &manager;
}

ChartManager::ChartManager() {
  ChartReport* report = new CollectionSizeReport;
  m_chartReports.insert(report->uuid(), report);
  report = new GroupSummaryReport;
  m_chartReports.insert(report->uuid(), report);
  report = new YearDistributionReport;
  m_chartReports.insert(report->uuid(), report);
}

ChartManager::~ChartManager() {
  qDeleteAll(m_chartReports);
  m_chartReports.clear();
}

QList<Tellico::ChartReport*> ChartManager::allReports() {
  return m_chartReports.values();
}

Tellico::ChartReport* ChartManager::report(const QUuid& uuid_) {
  return m_chartReports.value(uuid_);
}
