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

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class Iso6937Converter {
public:
  static QString toUtf8(const QCString& text);

private:
  static bool hasNext(unsigned int pos, unsigned int len);
  static bool isAscii(unsigned char c);
  static bool isCombining(unsigned char c);

  static QChar getChar(unsigned char c);
  static QChar getCombiningChar(unsigned int c);
};

} // end namespace

#endif
