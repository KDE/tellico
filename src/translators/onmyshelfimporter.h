/***************************************************************************
    Copyright (C) 2025 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMPORT_ONMYSHELFIMPORTER_H
#define TELLICO_IMPORT_ONMYSHELFIMPORTER_H

#include "importer.h"

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
*/
class OnMyShelfImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  OnMyShelfImporter(const QUrl& url);

  virtual Data::CollPtr collection() override;
  virtual bool canImport(int type) const override;

  virtual QWidget* widget(QWidget*) override { return nullptr; }

public Q_SLOTS:
  void slotCancel() override {}

private:
  void populateBooks(Data::EntryPtr entry, const QJsonObject& obj);
  void populateMovies(Data::EntryPtr entry, const QJsonObject& obj);
  void populateComics(Data::EntryPtr entry, const QJsonObject& obj);
  void populateBoardGames(Data::EntryPtr entry, const QJsonObject& obj);

  Data::CollPtr m_coll;
};

  } // end namespace
} // end namespace
#endif
