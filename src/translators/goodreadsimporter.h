/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMPORT_GOODREADSIMPORTER_H
#define TELLICO_IMPORT_GOODREADSIMPORTER_H

#include "importer.h"

#include <KSharedConfig>

class QLineEdit;

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
*/
class GoodreadsImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  GoodreadsImporter();

  virtual Data::CollPtr collection() override;
  virtual bool canImport(int type) const override;

  virtual QWidget* widget(QWidget* parent) override;

  void setConfig(KSharedConfig::Ptr config);

public Q_SLOTS:
  void slotCancel() override {}

private:
  QString text() const;
  QString idFromName(const QString& userName) const;

  Data::CollPtr m_coll;
  QWidget* m_widget;
  QLineEdit* m_userEdit;
  KSharedConfig::Ptr m_config;

  QUrl m_xsltURL;
  QString m_user;
  QString m_key;
};

  } // end namespace
} // end namespace
#endif
