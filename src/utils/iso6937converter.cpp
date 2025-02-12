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

// This class is adapted from Iso6937ToUnicode from the MARC4J project, available
// from https://github.com/marc4j/marc4j, with the following notice:
// * Copyright (C) 2002 Bas  Peters  (mail@bpeters.com)
// * Copyright (C) 2002 Yves Pratter (ypratter@club-internet.fr)
//
// That source was released under the terms of the GNU Lesser General Public
// License, version 2.1. In accordance with Condition 3 of that license,
// I am applying the terms of the GNU General Public License to the source
// code, and including a large portion of it here

#include "iso6937converter.h"
#include "../tellico_debug.h"

#include <QString>
#include <QByteArray>

using Tellico::Iso6937Converter;

QString Iso6937Converter::toUtf8(const QByteArray& text_) {
  const uint len = text_.length();
  QString result;
  result.reserve(len);
  for(uint i = 0; i < len; ++i) {
    uchar c = uchar(text_.at(i));
    if(isAscii(c)) {
      result.append(QLatin1Char(c));
    } else if(isCombining(c) && hasNext(i, len)) {
      const uchar next = uchar(text_.at(i+1));
      QChar d = getCombiningChar((c << 8) + next);
      if(!d.isNull()) {
        result.append(d);
        ++i;
      } else {
        result.append(getChar(c));
      }
    } else {
      result.append(getChar(c));
    }
  }
  result.squeeze();
  return result;
}

inline
bool Iso6937Converter::hasNext(uint pos, uint len) {
  return pos < (len - 1);
}

inline
bool Iso6937Converter::isAscii(uchar c) {
  return c <= 0x7F;
}

inline
bool Iso6937Converter::isCombining(uchar c) {
  return c >= 0xC0 && c <= 0xDF;
}

// Source : http://www.open-std.org/jtc1/sc2/wg3/docs/6937cd.pdf
QChar Iso6937Converter::getChar(uchar c) {
  return QChar(getCharInt(c));
}

int Iso6937Converter::getCharInt(uchar c) {
  switch(c) {
  case 0xA0:
    return 0x00A0; // 10/00 NO-BREAK SPACE
  case 0xA1:
    return 0x00A1; // 10/01 INVERTED EXCLAMATION MARK
  case 0xA2:
    return 0x00A2; // 10/02 CENT SIGN
  case 0xA3:
    return 0x00A3; // 10/03 POUND SIGN
    // 10/04 (This position shall not be used)
  case 0xA5:
    return 0x00A5; // 10/05 YEN SIGN
    // 10/06 (This position shall not be used)
  case 0xA7:
    return 0x00A7; // 10/07 SECTION SIGN
  case 0xA8:
    return 0x00A4; // 10/08 CURRENCY SIGN
  case 0xA9:
    return 0x2018; // 10/09 LEFT SINGLE QUOTATION MARK
  case 0xAA:
    return 0x201C; // 10/10 LEFT DOUBLE QUOTATION MARK
  case 0xAB:
    return 0x00AB; // 10/11 LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
  case 0xAC:
    return 0x2190; // 10/12 LEFTWARDS ARROW
  case 0xAD:
    return 0x2191; // 10/13 UPWARDS ARROW
  case 0xAE:
    return 0x2192; // 10/14 RIGHTWARDS ARROW
  case 0xAF:
    return 0x2193; // 10/15 DOWNWARDS ARROW

  case 0xB0:
    return 0x00B0; // 11/00 DEGREE SIGN
  case 0xB1:
    return 0x00B1; // 11/01 PLUS-MINUS SIGN
  case 0xB2:
    return 0x00B2; // 11/02 SUPERSCRIPT TWO
  case 0xB3:
    return 0x00B3; // 11/03 SUPERSCRIPT THREE
  case 0xB4:
    return 0x00D7; // 11/04 MULTIPLICATION SIGN
  case 0xB5:
    return 0x00B5; // 11/05 MICRO SIGN
  case 0xB6:
    return 0x00B6; // 11/06 PILCROW SIGN
  case 0xB7:
    return 0x00B7; // 11/07 MIDDLE DOT
  case 0xB8:
    return 0x00F7; // 11/08 DIVISION SIGN
  case 0xB9:
    return 0x2019; // 11/09 RIGHT SINGLE QUOTATION MARK
  case 0xBA:
    return 0x201D; // 11/10 RIGHT DOUBLE QUOTATION MARK
  case 0xBB:
    return 0x00BB; // 11/11 RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
  case 0xBC:
    return 0x00BC; // 11/12 VULGAR FRACTION ONE QUARTER
  case 0xBD:
    return 0x00BD; // 11/13 VULGAR FRACTION ONE HALF
  case 0xBE:
    return 0x00BE; // 11/14 VULGAR FRACTION THREE QUARTERS
  case 0xBF:
    return 0x00BF; // 11/15 INVERTED QUESTION MARK

    // 4/0 to 5/15 diacritic characters

  case 0xD0:
    return 0x2015; // 13/00 HORIZONTAL BAR
  case 0xD1:
    return 0x00B9; // 13/01 SUPERSCRIPT ONE
  case 0xD2:
    return 0x00AE; // 13/02 REGISTERED SIGN
  case 0xD3:
    return 0x00A9; // 13/03 COPYRIGHT SIGN
  case 0xD4:
    return 0x00AE; // 13/04 TRADE MARK SIGN
  case 0xD5:
    return 0x266A; // 13/05 EIGHTH NOTE
  case 0xD6:
    return 0x00AC; // 13/06 NOT SIGN
  case 0xD7:
    return 0x00A6; // 13/07 BROKEN BAR
    // 13/08 (This position shall not be used)
    // 13/09 (This position shall not be used)
    // 13/10 (This position shall not be used)
    // 13/11 (This position shall not be used)
  case 0xDC:
    return 0x215B; // 13/12 VULGAR FRACTION ONE EIGHTH
  case 0xDF:
    return 0x215E; // 13/15 VULGAR FRACTION SEVEN EIGHTHS

  case 0xE0:
    return 0x2126; // 14/00 OHM SIGN
  case 0xE1:
    return 0x00C6; // 14/01 LATIN CAPITAL LETTER AE
  case 0xE2:
    return 0x0110; // 14/02 LATIN CAPITAL LETTER D WITH STROKE
  case 0xE3:
    return 0x00AA; // 14/03 FEMININE ORDINAL INDICATOR
  case 0xE4:
    return 0x0126; // 14/04 LATIN CAPITAL LETTER H WITH STROKE
    // 14/05 (This position shall not be used)
  case 0xE6:
    return 0x0132; // 14/06 LATIN CAPITAL LIGATURE IJ
  case 0xE7:
    return 0x013F; // 14/07 LATIN CAPITAL LETTER L WITH MIDDLE DOT
  case 0xE8:
    return 0x0141; // 14/08 LATIN CAPITAL LETTER L WITH STROKE
  case 0xE9:
    return 0x00D8; // 14/09 LATIN CAPITAL LETTER O WITH STROKE
  case 0xEA:
    return 0x0152; // 14/10 LATIN CAPITAL LIGATURE OE
  case 0xEB:
    return 0x00BA; // 14/11 MASCULINE ORDINAL INDICATOR
  case 0xEC:
    return 0x00DE; // 14/12 LATIN CAPITAL LETTER THORN
  case 0xED:
    return 0x0166; // 14/13 LATIN CAPITAL LETTER T WITH STROKE
  case 0xEE:
    return 0x014A; // 14/14 LATIN CAPITAL LETTER ENG
  case 0xEF:
    return 0x0149; // 14/15 LATIN SMALL LETTER N PRECEDED BY APOSTROPHE

  case 0xF0:
    return 0x0138; // 15/00 LATIN SMALL LETTER KRA
  case 0xF1:
    return 0x00E6; // 15/01 LATIN SMALL LETTER AE
  case 0xF2:
    return 0x0111; // 15/02 LATIN SMALL LETTER D WITH STROKE
  case 0xF3:
    return 0x00F0; // 15/03 LATIN SMALL LETTER ETH
  case 0xF4:
    return 0x0127; // 15/04 LATIN SMALL LETTER H WITH STROKE
  case 0xF5:
    return 0x0131; // 15/05 LATIN SMALL LETTER DOTLESS I
  case 0xF6:
    return 0x0133; // 15/06 LATIN SMALL LIGATURE IJ
  case 0xF7:
    return 0x0140; // 15/07 LATIN SMALL LETTER L WITH MIDDLE DOT
  case 0xF8:
    return 0x0142; // 15/08 LATIN SMALL LETTER L WITH STROKE
  case 0xF9:
    return 0x00F8; // 15/09 LATIN SMALL LETTER O WITH STROKE
  case 0xFA:
    return 0x0153; // 15/10 LATIN SMALL LIGATURE OE
  case 0xFB:
    return 0x00DF; // 15/11 LATIN SMALL LETTER SHARP S
  case 0xFC:
    return 0x00FE; // 15/12 LATIN SMALL LETTER THORN
  case 0xFD:
    return 0x0167; // 15/13 LATIN SMALL LETTER T WITH STROKE
  case 0xFE:
    return 0x014B; // 15/14 LATIN SMALL LETTER ENG
  case 0xFF:
    return 0x00AD; // 15/15 SOFT HYPHEN$
  default:
    return c;
  }
}

QChar Iso6937Converter::getCombiningChar(uint c) {
  const auto cc = getCombiningCharInt(c);
  return cc ? QChar(cc) : QChar();
}

int Iso6937Converter::getCombiningCharInt(uint c) {
  switch(c) {
    // 12/00 (This position shall not be used)

    // 12/01 non-spacing grave accent
  case 0xC141:
    return 0x00C0; // LATIN CAPITAL LETTER A WITH GRAVE
  case 0xC145:
    return 0x00C8; // LATIN CAPITAL LETTER E WITH GRAVE
  case 0xC149:
    return 0x00CC; // LATIN CAPITAL LETTER I WITH GRAVE
  case 0xC14F:
    return 0x00D2; // LATIN CAPITAL LETTER O WITH GRAVE
  case 0xC155:
    return 0x00D9; // LATIN CAPITAL LETTER U WITH GRAVE
  case 0xC161:
    return 0x00E0; // LATIN SMALL LETTER A WITH GRAVE
  case 0xC165:
    return 0x00E8; // LATIN SMALL LETTER E WITH GRAVE
  case 0xC169:
    return 0x00EC; // LATIN SMALL LETTER I WITH GRAVE
  case 0xC16F:
    return 0x00F2; // LATIN SMALL LETTER O WITH GRAVE
  case 0xC175:
    return 0x00F9; // LATIN SMALL LETTER U WITH GRAVE

    // 12/02 non-spacing acute accent
  case 0xC220:
    return 0x00B4; // ACUTE ACCENT
  case 0xC241:
    return 0x00C1; // LATIN CAPITAL LETTER A WITH ACUTE
  case 0xC243:
    return 0x0106; // LATIN CAPITAL LETTER C WITH ACUTE
  case 0xC245:
    return 0x00C9; // LATIN CAPITAL LETTER E WITH ACUTE
  case 0xC249:
    return 0x00CD; // LATIN CAPITAL LETTER I WITH ACUTE
  case 0xC24C:
    return 0x0139; // LATIN CAPITAL LETTER L WITH ACUTE
  case 0xC24E:
    return 0x0143; // LATIN CAPITAL LETTER N WITH ACUTE
  case 0xC24F:
    return 0x00D3; // LATIN CAPITAL LETTER O WITH ACUTE
  case 0xC252:
    return 0x0154; // LATIN CAPITAL LETTER R WITH ACUTE
  case 0xC253:
    return 0x015A; // LATIN CAPITAL LETTER S WITH ACUTE
  case 0xC255:
    return 0x00DA; // LATIN CAPITAL LETTER U WITH ACUTE
  case 0xC259:
    return 0x00DD; // LATIN CAPITAL LETTER Y WITH ACUTE
  case 0xC25A:
    return 0x0179; // LATIN CAPITAL LETTER Z WITH ACUTE
  case 0xC261:
    return 0x00E1; // LATIN SMALL LETTER A WITH ACUTE
  case 0xC263:
    return 0x0107; // LATIN SMALL LETTER C WITH ACUTE
  case 0xC265:
    return 0x00E9; // LATIN SMALL LETTER E WITH ACUTE
  case 0xC267:
    return 0x01F5; // LATIN SMALL LETTER G WITH CEDILLA(4)
  case 0xC269:
    return 0x00ED; // LATIN SMALL LETTER I WITH ACUTE
  case 0xC26C:
    return 0x013A; // LATIN SMALL LETTER L WITH ACUTE
  case 0xC26E:
    return 0x0144; // LATIN SMALL LETTER N WITH ACUTE
  case 0xC26F:
    return 0x00F3; // LATIN SMALL LETTER O WITH ACUTE
  case 0xC272:
    return 0x0155; // LATIN SMALL LETTER R WITH ACUTE
  case 0xC273:
    return 0x015B; // LATIN SMALL LETTER S WITH ACUTE
  case 0xC275:
    return 0x00FA; // LATIN SMALL LETTER U WITH ACUTE
  case 0xC279:
    return 0x00FD; // LATIN SMALL LETTER Y WITH ACUTE
  case 0xC27A:
    return 0x017A; // LATIN SMALL LETTER Z WITH ACUTE

    // 12/03 non-spacing circumflex accent
  case 0xC341:
    return 0x00C2; // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
  case 0xC343:
    return 0x0108; // LATIN CAPITAL LETTER C WITH CIRCUMFLEX
  case 0xC345:
    return 0x00CA; // LATIN CAPITAL LETTER E WITH CIRCUMFLEX
  case 0xC347:
    return 0x011C; // LATIN CAPITAL LETTER G WITH CIRCUMFLEX
  case 0xC348:
    return 0x0124; // LATIN CAPITAL LETTER H WITH CIRCUMFLEX
  case 0xC349:
    return 0x00CE; // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
  case 0xC34A:
    return 0x0134; // LATIN CAPITAL LETTER J WITH CIRCUMFLEX
  case 0xC34F:
    return 0x00D4; // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
  case 0xC353:
    return 0x015C; // LATIN CAPITAL LETTER S WITH CIRCUMFLEX
  case 0xC355:
    return 0x00DB; // LATIN CAPITAL LETTER U WITH CIRCUMFLEX
  case 0xC357:
    return 0x0174; // LATIN CAPITAL LETTER W WITH CIRCUMFLEX
  case 0xC359:
    return 0x0176; // LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
  case 0xC361:
    return 0x00E2; // LATIN SMALL LETTER A WITH CIRCUMFLEX
  case 0xC363:
    return 0x0109; // LATIN SMALL LETTER C WITH CIRCUMFLEX
  case 0xC365:
    return 0x00EA; // LATIN SMALL LETTER E WITH CIRCUMFLEX
  case 0xC367:
    return 0x011D; // LATIN SMALL LETTER G WITH CIRCUMFLEX
  case 0xC368:
    return 0x0125; // LATIN SMALL LETTER H WITH CIRCUMFLEX
  case 0xC369:
    return 0x00EE; // LATIN SMALL LETTER I WITH CIRCUMFLEX
  case 0xC36A:
    return 0x0135; // LATIN SMALL LETTER J WITH CIRCUMFLEX
  case 0xC36F:
    return 0x00F4; // LATIN SMALL LETTER O WITH CIRCUMFLEX
  case 0xC373:
    return 0x015D; // LATIN SMALL LETTER S WITH CIRCUMFLEX
  case 0xC375:
    return 0x00FB; // LATIN SMALL LETTER U WITH CIRCUMFLEX
  case 0xC377:
    return 0x0175; // LATIN SMALL LETTER W WITH CIRCUMFLEX
  case 0xC379:
    return 0x0177; // LATIN SMALL LETTER Y WITH CIRCUMFLEX

    // 12/04 non-spacing tilde
  case 0xC441:
    return 0x00C3; // LATIN CAPITAL LETTER A WITH TILDE
  case 0xC449:
    return 0x0128; // LATIN CAPITAL LETTER I WITH TILDE
  case 0xC44E:
    return 0x00D1; // LATIN CAPITAL LETTER N WITH TILDE
  case 0xC44F:
    return 0x00D5; // LATIN CAPITAL LETTER O WITH TILDE
  case 0xC455:
    return 0x0168; // LATIN CAPITAL LETTER U WITH TILDE
  case 0xC461:
    return 0x00E3; // LATIN SMALL LETTER A WITH TILDE
  case 0xC469:
    return 0x0129; // LATIN SMALL LETTER I WITH TILDE
  case 0xC46E:
    return 0x00F1; // LATIN SMALL LETTER N WITH TILDE
  case 0xC46F:
    return 0x00F5; // LATIN SMALL LETTER O WITH TILDE
  case 0xC475:
    return 0x0169; // LATIN SMALL LETTER U WITH TILDE

    // 12/05 non-spacing macron
  case 0xC541:
    return 0x0100; // LATIN CAPITAL LETTER A WITH MACRON
  case 0xC545:
    return 0x0112; // LATIN CAPITAL LETTER E WITH MACRON
  case 0xC549:
    return 0x012A; // LATIN CAPITAL LETTER I WITH MACRON
  case 0xC54F:
    return 0x014C; // LATIN CAPITAL LETTER O WITH MACRON
  case 0xC555:
    return 0x016A; // LATIN CAPITAL LETTER U WITH MACRON
  case 0xC561:
    return 0x0101; // LATIN SMALL LETTER A WITH MACRON
  case 0xC565:
    return 0x0113; // LATIN SMALL LETTER E WITH MACRON
  case 0xC569:
    return 0x012B; // LATIN SMALL LETTER I WITH MACRON
  case 0xC56F:
    return 0x014D; // LATIN SMALL LETTER O WITH MACRON
  case 0xC575:
    return 0x016B; // LATIN SMALL LETTER U WITH MACRON

    // 12/06 non-spacing breve
  case 0xC620:
    return 0x02D8; // BREVE
  case 0xC641:
    return 0x0102; // LATIN CAPITAL LETTER A WITH BREVE
  case 0xC647:
    return 0x011E; // LATIN CAPITAL LETTER G WITH BREVE
  case 0xC655:
    return 0x016C; // LATIN CAPITAL LETTER U WITH BREVE
  case 0xC661:
    return 0x0103; // LATIN SMALL LETTER A WITH BREVE
  case 0xC667:
    return 0x011F; // LATIN SMALL LETTER G WITH BREVE
  case 0xC675:
    return 0x016D; // LATIN SMALL LETTER U WITH BREVE

    // 12/07 non-spacing dot above
  case 0xC743:
    return 0x010A; // LATIN CAPITAL LETTER C WITH DOT ABOVE
  case 0xC745:
    return 0x0116; // LATIN CAPITAL LETTER E WITH DOT ABOVE
  case 0xC747:
    return 0x0120; // LATIN CAPITAL LETTER G WITH DOT ABOVE
  case 0xC749:
    return 0x0130; // LATIN CAPITAL LETTER I WITH DOT ABOVE
  case 0xC75A:
    return 0x017B; // LATIN CAPITAL LETTER Z WITH DOT ABOVE
  case 0xC763:
    return 0x010B; // LATIN SMALL LETTER C WITH DOT ABOVE
  case 0xC765:
    return 0x0117; // LATIN SMALL LETTER E WITH DOT ABOVE
  case 0xC767:
    return 0x0121; // LATIN SMALL LETTER G WITH DOT ABOVE
  case 0xC77A:
    return 0x017C; // LATIN SMALL LETTER Z WITH DOT ABOVE

    // 12/08 non-spacing diaeresis
  case 0xC820:
    return 0x00A8; // DIAERESIS
  case 0xC841:
    return 0x00C4; // LATIN CAPITAL LETTER A WITH DIAERESIS
  case 0xC845:
    return 0x00CB; // LATIN CAPITAL LETTER E WITH DIAERESIS
  case 0xC849:
    return 0x00CF; // LATIN CAPITAL LETTER I WITH DIAERESIS
  case 0xC84F:
    return 0x00D6; // LATIN CAPITAL LETTER O WITH DIAERESIS
  case 0xC855:
    return 0x00DC; // LATIN CAPITAL LETTER U WITH DIAERESIS
  case 0xC859:
    return 0x0178; // LATIN CAPITAL LETTER Y WITH DIAERESIS
  case 0xC861:
    return 0x00E4; // LATIN SMALL LETTER A WITH DIAERESIS
  case 0xC865:
    return 0x00EB; // LATIN SMALL LETTER E WITH DIAERESIS
  case 0xC869:
    return 0x00EF; // LATIN SMALL LETTER I WITH DIAERESIS
  case 0xC86F:
    return 0x00F6; // LATIN SMALL LETTER O WITH DIAERESIS
  case 0xC875:
    return 0x00FC; // LATIN SMALL LETTER U WITH DIAERESIS
  case 0xC879:
    return 0x00FF; // LATIN SMALL LETTER Y WITH DIAERESIS

    // 12/09 (This position shall not be used)

    // 12/10 non-spacing ring above
  case 0xCA20:
    return 0x02DA; // RING ABOVE
  case 0xCA41:
    return 0x00C5; // LATIN CAPITAL LETTER A WITH RING ABOVE
  case 0xCA55:
    return 0x016E; // LATIN CAPITAL LETTER U WITH RING ABOVE
  case 0xCA61:
    return 0x00E5; // LATIN SMALL LETTER A WITH RING ABOVE
  case 0xCA75:
    return 0x016F; // LATIN SMALL LETTER U WITH RING ABOVE

    // 12/11 non-spacing cedilla
  case 0xCB20:
    return 0x00B8; // CEDILLA
  case 0xCB43:
    return 0x00C7; // LATIN CAPITAL LETTER C WITH CEDILLA
  case 0xCB47:
    return 0x0122; // LATIN CAPITAL LETTER G WITH CEDILLA
  case 0xCB4B:
    return 0x0136; // LATIN CAPITAL LETTER K WITH CEDILLA
  case 0xCB4C:
    return 0x013B; // LATIN CAPITAL LETTER L WITH CEDILLA
  case 0xCB4E:
    return 0x0145; // LATIN CAPITAL LETTER N WITH CEDILLA
  case 0xCB52:
    return 0x0156; // LATIN CAPITAL LETTER R WITH CEDILLA
  case 0xCB53:
    return 0x015E; // LATIN CAPITAL LETTER S WITH CEDILLA
  case 0xCB54:
    return 0x0162; // LATIN CAPITAL LETTER T WITH CEDILLA
  case 0xCB63:
    return 0x00E7; // LATIN SMALL LETTER C WITH CEDILLA
  case 0xCB67:
    return 0x0123; // small g with cedilla
  case 0xCB6B:
    return 0x0137; // LATIN SMALL LETTER K WITH CEDILLA
  case 0xCB6C:
    return 0x013C; // LATIN SMALL LETTER L WITH CEDILLA
  case 0xCB6E:
    return 0x0146; // LATIN SMALL LETTER N WITH CEDILLA
  case 0xCB72:
    return 0x0157; // LATIN SMALL LETTER R WITH CEDILLA
  case 0xCB73:
    return 0x015F; // LATIN SMALL LETTER S WITH CEDILLA
  case 0xCB74:
    return 0x0163; // LATIN SMALL LETTER T WITH CEDILLA

    // 12/12 (This position shall not be used)

    // 12/13 non-spacing double acute accent
  case 0xCD4F:
    return 0x0150; // LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
  case 0xCD55:
    return 0x0170; // LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
  case 0xCD6F:
    return 0x0151; // LATIN SMALL LETTER O WITH DOUBLE ACUTE
  case 0xCD75:
    return 0x0171; // LATIN SMALL LETTER U WITH DOUBLE ACUTE

    // 12/14 non-spacing ogonek
  case 0xCE20:
    return 0x02DB; // ogonek
  case 0xCE41:
    return 0x0104; // LATIN CAPITAL LETTER A WITH OGONEK
  case 0xCE45:
    return 0x0118; // LATIN CAPITAL LETTER E WITH OGONEK
  case 0xCE49:
    return 0x012E; // LATIN CAPITAL LETTER I WITH OGONEK
  case 0xCE55:
    return 0x0172; // LATIN CAPITAL LETTER U WITH OGONEK
  case 0xCE61:
    return 0x0105; // LATIN SMALL LETTER A WITH OGONEK
  case 0xCE65:
    return 0x0119; // LATIN SMALL LETTER E WITH OGONEK
  case 0xCE69:
    return 0x012F; // LATIN SMALL LETTER I WITH OGONEK
  case 0xCE75:
    return 0x0173; // LATIN SMALL LETTER U WITH OGONEK

    // 12/15 non-spacing caron
  case 0xCF20:
    return 0x02C7; // CARON
  case 0xCF43:
    return 0x010C; // LATIN CAPITAL LETTER C WITH CARON
  case 0xCF44:
    return 0x010E; // LATIN CAPITAL LETTER D WITH CARON
  case 0xCF45:
    return 0x011A; // LATIN CAPITAL LETTER E WITH CARON
  case 0xCF4C:
    return 0x013D; // LATIN CAPITAL LETTER L WITH CARON
  case 0xCF4E:
    return 0x0147; // LATIN CAPITAL LETTER N WITH CARON
  case 0xCF52:
    return 0x0158; // LATIN CAPITAL LETTER R WITH CARON
  case 0xCF53:
    return 0x0160; // LATIN CAPITAL LETTER S WITH CARON
  case 0xCF54:
    return 0x0164; // LATIN CAPITAL LETTER T WITH CARON
  case 0xCF5A:
    return 0x017D; // LATIN CAPITAL LETTER Z WITH CARON
  case 0xCF63:
    return 0x010D; // LATIN SMALL LETTER C WITH CARON
  case 0xCF64:
    return 0x010F; // LATIN SMALL LETTER D WITH CARON
  case 0xCF65:
    return 0x011B; // LATIN SMALL LETTER E WITH CARON
  case 0xCF6C:
    return 0x013E; // LATIN SMALL LETTER L WITH CARON
  case 0xCF6E:
    return 0x0148; // LATIN SMALL LETTER N WITH CARON
  case 0xCF72:
    return 0x0159; // LATIN SMALL LETTER R WITH CARON
  case 0xCF73:
    return 0x0161; // LATIN SMALL LETTER S WITH CARON
  case 0xCF74:
    return 0x0165; // LATIN SMALL LETTER T WITH CARON
  case 0xCF7A:
    return 0x017E; // LATIN SMALL LETTER Z WITH CARON

  default:
    myDebug() << "no match for" << Qt::hex << c;
    return 0;
  }
}
