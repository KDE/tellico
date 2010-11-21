/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMPORT_TELLICOXMLHANDLER_H
#define TELLICO_IMPORT_TELLICOXMLHANDLER_H

#include "xmlstatehandler.h"

#include <QStack>

namespace Tellico {
  namespace Import {

class TellicoXMLHandler : public QXmlDefaultHandler {
public:
  TellicoXMLHandler();
  ~TellicoXMLHandler();

  virtual bool startElement(const QString& namespaceURI, const QString& localName,
                            const QString& qName, const QXmlAttributes& atts);
  virtual bool endElement(const QString& namespaceURI, const QString& localName,
                          const QString& qName);
  virtual bool characters(const QString& ch);

  virtual QString errorString() const;

  Data::CollPtr collection() const;
  bool hasImages() const;

  void setLoadImages(bool loadImages);
  void setShowImageLoadErrors(bool showImageErrors);

private:
  QStack<SAX::StateHandler*> m_handlers;
  SAX::StateData* m_data;
};

  }
}
#endif
