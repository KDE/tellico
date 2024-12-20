/***************************************************************************
    Copyright (C) 2022 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_MARCIMPORTER_H
#define TELLICO_MARCIMPORTER_H

#include "importer.h"
#include "../datavectors.h"

#include <QString>

class KComboBox;

namespace Tellico {
  class XSLTHandler;
  namespace Import {

/**
 * @author Robby Stephenson
 */
class MarcImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  MarcImporter(const QUrl& url_);
  ~MarcImporter();

  /**
   * @return A pointer to a @ref Data::Collection, or 0 if none can be created.
   */
  virtual Data::CollPtr collection() override;
  /**
   */
  virtual QWidget* widget(QWidget*) override;
  virtual bool canImport(int type) const override;
  void setCharacterSet(const QString& charSet);

public Q_SLOTS:
  void slotCancel() override;

private:
  bool initMARCHandler();
  bool initMODSHandler();

  Data::CollPtr m_coll;
  bool m_cancelled;

  QString m_marcdump;
  QString m_marcCharSet;
  bool m_isUnimarc;
  XSLTHandler* m_MARCHandler;
  XSLTHandler* m_MODSHandler;

  QWidget* m_widget;
  KComboBox* m_charSetCombo;
  KComboBox* m_marcFormatCombo;
};

  } // end namespace
} // end namespace
#endif
