/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMPORT_GCSTARIMPORTER_H
#define TELLICO_IMPORT_GCSTARIMPORTER_H

#include "textimporter.h"
#include "../datavectors.h"

class QRegExp;

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
*/
class GCstarImporter : public TextImporter {
Q_OBJECT

public:
  /**
   */
  GCstarImporter(const KUrl& url);
  GCstarImporter(const QString& text);

  /**
   *
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget*) { return 0; }
  virtual bool canImport(int type) const;

public slots:
  void slotCancel();

private:
  static QString splitJoin(const QRegExp& rx, const QString& s);

  void readGCfilms(const QString& text);
  void readGCstar(const QString& text);

  Data::CollPtr m_coll;
  bool m_cancelled;
};

  } // end namespace
} // end namespace
#endif
