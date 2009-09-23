/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_PILOTDBEXPORTER_H
#define TELLICO_PILOTDBEXPORTER_H

class QCheckBox;

#include "exporter.h"

#include <QStringList>

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class PilotDBExporter : public Exporter {
Q_OBJECT

public:
  PilotDBExporter(Data::CollPtr coll);

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  virtual QWidget* widget(QWidget* parent);
  virtual void readOptions(KSharedConfigPtr cfg);
  virtual void saveOptions(KSharedConfigPtr cfg);

  void setColumns(const QStringList& columns) { m_columns = columns; }

private:
  bool m_backup;

  QWidget* m_widget;
  QCheckBox* m_checkBackup;
  QStringList m_columns;
};

  } // end namespace
} // end namespace
#endif
