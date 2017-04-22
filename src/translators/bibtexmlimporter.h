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

#ifndef TELLICO_BIBTEXMLIMPORTER_H
#define TELLICO_BIBTEXMLIMPORTER_H

#include "xmlimporter.h"
#include "../datavectors.h"

class QDomNode;

namespace Tellico {
  namespace Import {

/**
 *@author Robby Stephenson
 */
class BibtexmlImporter : public XMLImporter {
Q_OBJECT

public:
  /**
   */
  BibtexmlImporter(const QUrl& url);
  BibtexmlImporter(const QString& text);

  /**
   */
  virtual Data::CollPtr collection() Q_DECL_OVERRIDE;
  virtual bool canImport(int type) const Q_DECL_OVERRIDE;

public Q_SLOTS:
  void slotCancel();

private:
  void loadDomDocument();
  void readEntry(const QDomNode& entryNode);

  Data::CollPtr m_coll;
  bool m_cancelled : 1;
};

  } // end namespace
} // end namespace
#endif
