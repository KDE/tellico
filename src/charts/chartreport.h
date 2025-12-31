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

#ifndef TELLICO_CHARTREPORT_H
#define TELLICO_CHARTREPORT_H

class QWidget;
#include <QUuid>

namespace Tellico {

/**
 * The ChartReport class provides a widget showing one or more charts as a report.
 *
 * @author Robby Stephenson
 */
class ChartReport {

public:
  ChartReport() {}
  virtual ~ChartReport() {}

  QUuid uuid() { if(m_uuid.isNull()) m_uuid = QUuid::createUuid(); return m_uuid; }
  virtual QString title() const = 0;
  // the calling function is responsible for deleting the widget
  virtual QWidget* createWidget() = 0;

private:
  Q_DISABLE_COPY(ChartReport)

  QUuid m_uuid;
};

} // end namespace
#endif
