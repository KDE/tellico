/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_TELLICOXMLREADER_H
#define TELLICO_TELLICOXMLREADER_H

#include "xmlstatehandler.h"

#include <QXmlStreamReader>
#include <QStack>

namespace Tellico {
  namespace Import {

class TellicoXmlReader {
public:
  TellicoXmlReader(const QUrl& baseUrl);
  ~TellicoXmlReader();

  bool readNext(const QByteArray& data);
  QString errorString() const;
  bool isNotWellFormed() const;

  Data::CollPtr collection() const;
  bool hasImages() const;

  void setLoadImages(bool loadImages);
  void setShowImageLoadErrors(bool showImageErrors);
  void setImagePathsAsLinks(bool imagePathsAsLinks);

private:
  void handleStart();
  void handleEnd();
  void handleCharacters();

  QXmlStreamReader m_xml;
  QStack<SAX::StateHandler*> m_handlers;
  SAX::StateData* m_data;
};

  }
}
#endif
