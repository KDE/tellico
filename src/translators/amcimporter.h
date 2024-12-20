/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMPORT_AMCIMPORTER_H
#define TELLICO_IMPORT_AMCIMPORTER_H

#include "dataimporter.h"
#include <QDataStream>

namespace Tellico {
  namespace Import {

/**
 @author Robby Stephenson
 */
class AMCImporter : public DataImporter {
Q_OBJECT
public:
  AMCImporter(const QUrl& url);
  virtual ~AMCImporter();

  virtual Data::CollPtr collection() override;
  bool canImport(int type) const override;

public Q_SLOTS:
  void slotCancel() override;

private:
  bool readBool();
  quint32 readInt();
  QString readString();
  QString readImage(const QString& format);
  void readEntry();
  QStringList parseCast(const QString& text);

  Data::CollPtr m_coll;
  bool m_cancelled;
  bool m_failed;
  QDataStream m_ds;
  int m_majVersion;
  int m_minVersion;
};

  } // end namespace
} // end namespace

#endif
