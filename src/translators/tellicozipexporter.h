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

#ifndef TELLICO_ZIPEXPORTER_H
#define TELLICO_ZIPEXPORTER_H

#include "exporter.h"

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class TellicoZipExporter : public Exporter {
Q_OBJECT

public:
  TellicoZipExporter(Data::CollPtr coll);

  virtual bool exec() override;
  virtual QString formatString() const override;
  virtual QString fileFilter() const override;

  // no options
  virtual QWidget* widget(QWidget*) override { return nullptr; }

  void setIncludeImages(bool b) { m_includeImages = b; }

public Q_SLOTS:
  void slotCancel();

private:
  bool m_includeImages : 1;
  bool m_cancelled : 1;
};

  } // end namespace
} // end namespace
#endif
