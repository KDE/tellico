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

#ifndef ALEXANDRIAIMPORTER_H
#define ALEXANDRIAIMPORTER_H

class KComboBox;

#include "importer.h"
#include "../datavectors.h"

#include <QDir>

namespace Tellico {
  namespace Import {

/**
 * An importer for importing collections used by Alexandria, the Gnome book collection manager.
 *
 * The libraries are assumed to be in $HOME/.alexandria. The file format is YAML, but instead
 * using a real YAML reader, the file is parsed line-by-line, so it's very crude. When Alexandria
 * adds new fields or types, this will have to be updated.
 *
 * @author Robby Stephenson
 */
class AlexandriaImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  AlexandriaImporter();
  /**
   */
  virtual ~AlexandriaImporter() {}

  /**
   */
  virtual Data::CollPtr collection() override;
  /**
   */
  virtual QWidget* widget(QWidget* parent) override;
  virtual bool canImport(int type) const override;

  void setLibraryPath(const QString& libraryPath) { m_libraryPath = libraryPath; }
  QString libraryPath() const { return m_libraryPath; }

public Q_SLOTS:
  void slotCancel() override;

private:
  static QString& cleanLine(QString& str);
  static QString& clean(QString& str);

  Data::CollPtr m_coll;
  QWidget* m_widget;
  KComboBox* m_library;
  QString m_libraryPath;

  QDir m_libraryDir;
  bool m_cancelled;
};

  } // end namespace
} // end namespace
#endif
