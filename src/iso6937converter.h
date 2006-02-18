/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_ISO6937CONVERTER_H
#define TELLICO_ISO6937CONVERTER_H

class QCString;
class QString;
class QChar;

#include <qglobal.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class Iso6937Converter {
public:
  static QString toUtf8(const QCString& text);

private:
  static bool hasNext(uint pos, uint len);
  static bool isAscii(uchar c);
  static bool isCombining(uchar c);

  static QChar getChar(uchar c);
  static QChar getCombiningChar(uint c);
};

} // end namespace

#endif
