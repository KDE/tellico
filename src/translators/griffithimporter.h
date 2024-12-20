/***************************************************************************
    Copyright (C) 2007-2012 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_GRIFFITHIMPORTER_H
#define TELLICO_GRIFFITHIMPORTER_H

#include "xsltimporter.h"

namespace Tellico {
  namespace Import {

/**
 * An importer for importing collections used by Griffith, a movie collection manager.
 *
 * @author Robby Stephenson
 */
class GriffithImporter : public XSLTImporter {
Q_OBJECT

public:
  /**
   */
  GriffithImporter(const QUrl& url);
  /**
   */
  virtual ~GriffithImporter();

  virtual bool canImport(int type) const override;
  virtual void beginXSLTHandler(XSLTHandler* handler) override;
  virtual QWidget* widget(QWidget*) override { return nullptr; }

private:
  Data::CollPtr m_coll;
};

  } // end namespace
} // end namespace
#endif
