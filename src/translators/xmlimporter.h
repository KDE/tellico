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

#ifndef TELLICO_XMLIMPORTER_H
#define TELLICO_XMLIMPORTER_H

#include "importer.h"

#include <qdom.h>

namespace Tellico {
  namespace Import {

/**
 * The XMLImporter class is meant as an abstract class for any importer which reads xml files.
 *
 * @author Robby Stephenson
 */
class XMLImporter : public Importer {
Q_OBJECT

public:
  /**
   * In the constructor, the contents of the file are read.
   *
   * @param url The file to be imported
   */
  XMLImporter(const QUrl& url);
  /**
   * Imports xml text.
   *
   * @param text The text
   */
  XMLImporter(const QString& text);
  /**
   * Imports xml text from a byte array.
   *
   * @param data The Data
   */
  XMLImporter(const QByteArray& data);
  XMLImporter(const QDomDocument& dom);

  virtual void setText(const QString& text) Q_DECL_OVERRIDE;

  /**
   * This class gets used as a utility XML loader. This should never get called,
   * but cannot be abstract.
   */
  virtual Data::CollPtr collection() Q_DECL_OVERRIDE;
  virtual bool canImport(int type) const Q_DECL_OVERRIDE { Q_UNUSED(type); return true; }

  /**
   * Returns the contents of the imported file.
   *
   * @return The file contents
   */
  const QDomDocument& domDocument() const { return m_dom; }

public Q_SLOTS:
  void slotCancel() Q_DECL_OVERRIDE {}

private:
  QDomDocument m_dom;
};

  } // end namespace
} // end namespace
#endif