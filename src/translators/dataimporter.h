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

#ifndef TELLICO_DATAIMPORTER_H
#define TELLICO_DATAIMPORTER_H

#include "importer.h"
#include "../core/filehandler.h"

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
 */
class DataImporter : public Importer {
Q_OBJECT

public:
  enum Source { URL, Text };

  /**
   * @param url The URL of the file to import
   */
  DataImporter(const QUrl& url);
  /**
   * Since the conversion to a QCString appends a \0 character at the end, remove it.
   *
   * @param text The text. It MUST be in UTF-8.
   */
  DataImporter(const QString& text);

  /**
   */
  virtual ~DataImporter();

  Source source() const { return m_source; }

  virtual void setText(const QString& text) override;

protected:
  /**
   * Return the data in the imported file
   *
   * @return the file data
   */
  const QByteArray& data() const { return m_data; }
  FileHandler::FileRef& fileRef() const;

private:
  QByteArray m_data;
  Source m_source;
  FileHandler::FileRef* m_fileRef;
};

  } // end namespace
} // end namespace
#endif
