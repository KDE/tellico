/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ISO5426CONVERTER_H
#define TELLICO_ISO5426CONVERTER_H

class QByteArray;
class QString;
class QChar;

#include <qglobal.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class Iso5426Converter {
public:
  static QString toUtf8(const QByteArray& text);

private:
  static bool hasNext(unsigned int pos, unsigned int len);
  static bool isAscii(unsigned char c);
  static bool isCombining(unsigned char c);

  static QChar getChar(unsigned char c);
  static int getCharInt(unsigned char c);
  static QChar getCombiningChar(unsigned int c);
  static int getCombiningCharInt(unsigned int c);
};

} // end namespace

#endif
