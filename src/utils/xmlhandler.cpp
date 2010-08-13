/***************************************************************************
    Copyright (C) 2010 Robby Stephenson <robby@periapsis.org>
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

#include "xmlhandler.h"

#include <QTextStream>
#include <QXmlInputSource>

using Tellico::XMLHandler;

bool XMLHandler::setUtf8XmlEncoding(QString& text_) {
  static const QRegExp rx(QLatin1String("encoding\\s*=\\s*\"([\\w-]+)\""));
  QTextStream stream(&text_);
  // at this point, we read the data into a QString and plan to later convert to utf-8
  // but the xml header might still indicate an encoding other than utf-8
  // so, just to be safe, if the xml header is the first line, ensure it is set to utf-8
  QString firstLine = stream.readLine();
  if(rx.indexIn(firstLine) > -1) {
    if(rx.cap(1).toLower() != QLatin1String("utf-8")) {
      firstLine.replace(rx, QLatin1String("encoding=\"utf-8\""));
      text_ = firstLine + QLatin1Char('\n') + stream.readAll();
      return true;
    }
  }
  return false;
}

QString XMLHandler::readXMLData(const QByteArray& data_) {
  QXmlInputSource source;
  source.setData(data_);
  return source.data();
}
