/***************************************************************************
    Copyright (C) 2024 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FILEREADER_H
#define TELLICO_FILEREADER_H

#include "../datavectors.h"

#include <QUrl>

#include <memory>

class KFileItem;

namespace Tellico {

class AbstractFileReader {
public:
  AbstractFileReader(const QUrl& u) : m_url(u) {}
  virtual ~AbstractFileReader() {}

  QUrl url() const { return m_url; }

  virtual bool populate(Data::EntryPtr entry, const KFileItem& fileItem) = 0;

private:
  QUrl m_url;
};

class FileReaderPrivate;
class FileReader : public AbstractFileReader {
public:
  FileReader(const QUrl& u);
  virtual ~FileReader();

  virtual bool populate(Data::EntryPtr entry, const KFileItem& fileItem);

  void setUseFilePreview(bool filePreview);

private:
  QString volumeName() const;
  class Private;
  friend class Private;
  std::unique_ptr<Private> d;
};

}
#endif
