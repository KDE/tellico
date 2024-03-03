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

// This class is adapted from Iso5426ToUnicode from the MARC4J project, available
// from https://github.com/marc4j/marc4j, with the following notice:
// * Copyright (C) 2002 Bas  Peters  (mail@bpeters.com)
// * Copyright (C) 2002 Yves Pratter (ypratter@club-internet.fr)
//
// That source was released under the terms of the GNU Lesser General Public
// License, version 2.1. In accordance with Condition 3 of that license,
// I am applying the terms of the GNU General Public License to the source
// code, and including a large portion of it here

#include "iso5426converter.h"
#include "../tellico_debug.h"

#include <QString>
#include <QByteArray>

using Tellico::Iso5426Converter;

QString Iso5426Converter::toUtf8(const QByteArray& text_) {
  const uint len = text_.length();
  QString result;
  result.reserve(len);
  for(uint i = 0; i < len; ++i) {
    uchar c = uchar(text_.at(i));
    if(isAscii(c)) {
      result.append(QLatin1Char(c));
    } else if(isCombining(c) && hasNext(i, len)) {
      // this is a hack
      // use the diaeresis instead of umlaut
      // works for SUDOC
      if(c == 0xC9) {
        c = 0xC8;
      }
      // could be two combining characters
      uint comb = c << 8;
      uint skip = 1;
      const uchar next = uchar(text_.at(i+1));
      if(isCombining(next) && hasNext(i+1, len)) {
        comb = (comb + next) << 8;
        skip++;
      }
      comb += uchar(text_.at(i+skip));
      QChar d = getCombiningChar(comb);
      if(!d.isNull()) {
        result.append(d);
        i += skip;
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
bool Iso5426Converter::hasNext(uint pos, uint len) {
  return pos < (len - 1);
}

inline
bool Iso5426Converter::isAscii(uchar c) {
  return c <= 0x7F;
}

inline
bool Iso5426Converter::isCombining(uchar c) {
  return c >= 0xC0 && c <= 0xDF;
}

// Source : https://www.itscj.ipsj.or.jp/iso-ir/053.pdf
QChar Iso5426Converter::getChar(uchar c) {
  return QChar(getCharInt(c));
}

int Iso5426Converter::getCharInt(uchar c) {
  switch(c) {
  case 0xA1:
    return 0x00A1; // 2/1 inverted exclamation mark
  case 0xA2:
    return 0x201E; // 2/2 left low double quotation mark // was 0x201C
  case 0xA3:
    return 0x00A3; // 2/3 pound sign
  case 0xA4:
    return 0x0024; // 2/4 dollar sign
  case 0xA5:
    return 0x00A5; // 2/5 yen sign
  case 0xA6:
    return 0x2020; // 2/6 single dagger
  case 0xA7:
    return 0x00A7; // 2/7 paragraph (section)
  case 0xA8:
    return 0x2032; // 2/8 prime
  case 0xA9:
    return 0x2018; // 2/9 left high single quotation mark
  case 0xAA:
    return 0x201C; // 2/10 left high double quotation mark
  case 0xAB:
    return 0x00AB; // 2/11 left angle quotation mark
  case 0xAC:
    return 0x266D; // 2/12 music flat
  case 0xAD:
    return 0x00A9; // 2/13 copyright sign
  case 0xAE:
    return 0x2117; // 2/14 sound recording copyright sign
  case 0xAF:
    return 0x00AE; // 2/15 trade mark sign

  case 0xB0:
    return 0x0639; // 3/0 ayn [ain]
  case 0xB1:
    return 0x0623; // 3/1 alif/hamzah [alef with hamza above]
  case 0xB2:
    return 0x201A; // 3/2 left low single quotation mark // was 0x2018
  // 3/3 (this position shall not be used)
  // 3/4 (this position shall not be used)
  // 3/5 (this position shall not be used)
  case 0xB6:
    return 0x2021; // 3/6 double dagger
  case 0xB7:
    return 0x00B7; // 3/7 middle dot
  case 0xB8:
    return 0x2033; // 3/8 double prime
  case 0xB9:
    return 0x2019; // 3/9 right high single quotation mark
  case 0xBA:
    return 0x201D; // 3/10 right high double quotation mark
  case 0xBB:
    return 0x00BB; // 3/11 right angle quotation mark
  case 0xBC:
    return 0x266F; // 3/12 musical sharp
  case 0xBD:
    return 0x02B9; // 3/13 mjagkij znak
  case 0xBE:
    return 0x02BA; // 3/14 tverdyj znak
  case 0xBF:
    return 0x00BF; // 3/15 inverted question mark

  // 4/0 to 5/15 diacritic characters

  // 6/0 (this position shall not be used)
  case 0xE1:
    return 0x00C6; // 6/1 CAPITAL DIPHTHONG A WITH E
  case 0xE2:
    return 0x0110; // 6/2 CAPITAL LETTER D WITH STROKE
  // 6/3 (this position shall not be used)
  // 6/4 (this position shall not be used)
  // 6/5 (this position shall not be used)
  case 0xE6:
    return 0x0132; // 6/6 CAPITAL LETTER IJ
  // 6/7 (this position shall not be used)
  case 0xE8:
    return 0x0141; // 6/8 CAPITAL LETTER L WITH STROKE
  case 0xE9:
    return 0x00D8; // 6/9 CAPITAL LETTER O WITH SOLIDUS [oblique stroke]
  case 0xEA:
    return 0x0152; // 6/10 CAPITAL DIPHTONG OE
  // 6/11 (this position shall not be used)
  case 0xEC:
    return 0x00DE; // 6/12 CAPITAL LETTER THORN
  // 6/13 (this position shall not be used)
  // 6/14 (this position shall not be used)
  // 6/15 (this position shall not be used)

  // 7/0 (this position shall not be used)
  case 0xF1:
    return 0x00E6; // 7/1 small diphthong a with e
  case 0xF2:
    return 0x0111; // small letter d with stroke
  case 0xF3:
    return 0x00F0; // small letter eth
  // 7/4 (this position shall not be used)
  case 0xF5:
    return 0x0131; // 7/5 small letter i without dot
  case 0xF6:
    return 0x0133; // 7/6 small letter ij
  // 7/7 (this position shall not be used)
  case 0xF8:
    return 0x0142; // 7/8 small letter l with stroke
  case 0xF9:
    return 0x00F8; // 7/9 small letter o with solidus (oblique stroke)
  case 0xFA:
    return 0x0153; // 7/10 small diphtong oe
  case 0xFB:
    return 0x00DF; // 7/11 small letter sharp s
  case 0xFC:
    return 0x00FE; // 7/12 small letter thorn
  // 7/13 (this position shall not be used)
  // 7/14 (this position shall not be used)
  default:
    return c;
  }
}

QChar Iso5426Converter::getCombiningChar(uint c) {
  const auto cc = getCombiningCharInt(c);
  return cc ? QChar(cc) : QChar();
}

int Iso5426Converter::getCombiningCharInt(uint c) {
  switch(c) {
  // 4/0 low rising tone mark
  case 0xC041:
    return 0x1EA2; // CAPITAL A WITH HOOK ABOVE
  case 0xC045:
    return 0x1EBA; // CAPITAL E WITH HOOK ABOVE
  case 0xC049:
    return 0x1EC8; // CAPITAL I WITH HOOK ABOVE
  case 0xC04F:
    return 0x1ECE; // CAPITAL O WITH HOOK ABOVE
  case 0xC055:
    return 0x1EE6; // CAPITAL U WITH HOOK ABOVE
  case 0xC059:
    return 0x1EF6; // CAPITAL Y WITH HOOK ABOVE
  case 0xC061:
    return 0x1EA3; // small a with hook above
  case 0xC065:
    return 0x1EBB; // small e with hook above
  case 0xC069:
    return 0x1EC9; // small i with hook above
  case 0xC06F:
    return 0x1ECF; // small o with hook above
  case 0xC075:
    return 0x1EE7; // small u with hook above
  case 0xC079:
    return 0x1EF7; // small y with hook above

  // 4/1 grave accent
  case 0xC141:
    return 0x00C0; // CAPITAL A WITH GRAVE ACCENT
  case 0xC145:
    return 0x00C8; // CAPITAL E WITH GRAVE ACCENT
  case 0xC149:
    return 0x00CC; // CAPITAL I WITH GRAVE ACCENT
  case 0xC14E:
    return 0x01F8; // CAPITAL LETTER N WITH GRAVE
  case 0xC14F:
    return 0x00D2; // CAPITAL O WITH GRAVE ACCENT
  case 0xC155:
    return 0x00D9; // CAPITAL U WITH GRAVE ACCENT
  case 0xC157:
    return 0x1E80; // CAPITAL W WITH GRAVE
  case 0xC159:
    return 0x1EF2; // CAPITAL Y WITH GRAVE
  case 0xC161:
    return 0x00E0; // small a with grave accent
  case 0xC165:
    return 0x00E8; // small e with grave accent
  case 0xC169:
    return 0x00EC; // small i with grave accent
  case 0xC16E:
    return 0x01F9; // SMALL LETTER N WITH GRAVE
  case 0xC16F:
    return 0x00F2; // small o with grave accent
  case 0xC175:
    return 0x00F9; // small u with grave accent
  case 0xC177:
    return 0x1E81; // small w with grave
  case 0xC179:
    return 0x1EF3; // small y with grave

  // 4/2 acute accent
  case 0xC241:
    return 0x00C1; // CAPITAL A WITH ACUTE ACCENT
  case 0xC243:
    return 0x0106; // CAPITAL C WITH ACUTE ACCENT
  case 0xC245:
    return 0x00C9; // CAPITAL E WITH ACUTE ACCENT
  case 0xC247:
    return 0x01F4; // CAPITAL G WITH ACUTE
  case 0xC249:
    return 0x00CD; // CAPITAL I WITH ACUTE ACCENT
  case 0xC24B:
    return 0x1E30; // CAPITAL K WITH ACUTE
  case 0xC24C:
    return 0x0139; // CAPITAL L WITH ACUTE ACCENT
  case 0xC24D:
    return 0x1E3E; // CAPITAL M WITH ACUTE
  case 0xC24E:
    return 0x0143; // CAPITAL N WITH ACUTE ACCENT
  case 0xC24F:
    return 0x00D3; // CAPITAL O WITH ACUTE ACCENT
  case 0xC250:
    return 0x1E54; // CAPITAL P WITH ACUTE
  case 0xC252:
    return 0x0154; // CAPITAL R WITH ACUTE ACCENT
  case 0xC253:
    return 0x015A; // CAPITAL S WITH ACUTE ACCENT
  case 0xC255:
    return 0x00DA; // CAPITAL U WITH ACUTE ACCENT
  case 0xC257:
    return 0x1E82; // CAPITAL W WITH ACUTE
  case 0xC259:
    return 0x00DD; // CAPITAL Y WITH ACUTE ACCENT
  case 0xC25A:
    return 0x0179; // CAPITAL Z WITH ACUTE ACCENT
  case 0xC261:
    return 0x00E1; // small a with acute accent
  case 0xC263:
    return 0x0107; // small c with acute accent
  case 0xC265:
    return 0x00E9; // small e with acute accent
  case 0xC267:
    return 0x01F5; // small g with acute
  case 0xC269:
    return 0x00ED; // small i with acute accent
  case 0xC26B:
    return 0x1E31; // small k with acute
  case 0xC26C:
    return 0x013A; // small l with acute accent
  case 0xC26D:
    return 0x1E3F; // small m with acute
  case 0xC26E:
    return 0x0144; // small n with acute accent
  case 0xC26F:
    return 0x00F3; // small o with acute accent
  case 0xC270:
    return 0x1E55; // small p with acute
  case 0xC272:
    return 0x0155; // small r with acute accent
  case 0xC273:
    return 0x015B; // small s with acute accent
  case 0xC275:
    return 0x00FA; // small u with acute accent
  case 0xC277:
    return 0x1E83; // small w with acute
  case 0xC279:
    return 0x00FD; // small y with acute accent
  case 0xC27A:
    return 0x017A; // small z with acute accent
  case 0xC2E1:
    return 0x01FC; // CAPITAL AE WITH ACUTE
  case 0xC2E9:
    return 0x01FE; // CAPITAL LETTER O WITH STROKE AND ACUTE
  case 0xC2F1:
    return 0x01FD; // SMALL LETTER AE WITH ACUTE
  case 0xC2F9:
    return 0x01FF; // Small LETTER O WITH STROKE AND ACUTE

  // 4/3 circumflex accent
  case 0xC341:
    return 0x00C2; // CAPITAL A WITH CIRCUMFLEX ACCENT
  case 0xC343:
    return 0x0108; // CAPITAL C WITH CIRCUMFLEX
  case 0xC345:
    return 0x00CA; // CAPITAL E WITH CIRCUMFLEX ACCENT
  case 0xC347:
    return 0x011C; // CAPITAL G WITH CIRCUMFLEX
  case 0xC348:
    return 0x0124; // CAPITAL H WITH CIRCUMFLEX
  case 0xC349:
    return 0x00CE; // CAPITAL I WITH CIRCUMFLEX ACCENT
  case 0xC34A:
    return 0x0134; // CAPITAL J WITH CIRCUMFLEX
  case 0xC34F:
    return 0x00D4; // CAPITAL O WITH CIRCUMFLEX ACCENT
  case 0xC353:
    return 0x015C; // CAPITAL S WITH CIRCUMFLEX
  case 0xC355:
    return 0x00DB; // CAPITAL U WITH CIRCUMFLEX
  case 0xC357:
    return 0x0174; // CAPITAL W WITH CIRCUMFLEX
  case 0xC359:
    return 0x0176; // CAPITAL Y WITH CIRCUMFLEX
  case 0xC35A:
    return 0x1E90; // CAPITAL Z WITH CIRCUMFLEX
  case 0xC361:
    return 0x00E2; // small a with circumflex accent
  case 0xC363:
    return 0x0109; // small c with circumflex
  case 0xC365:
    return 0x00EA; // small e with circumflex accent
  case 0xC367:
    return 0x011D; // small g with circumflex
  case 0xC368:
    return 0x0125; // small h with circumflex
  case 0xC369:
    return 0x00EE; // small i with circumflex accent
  case 0xC36A:
    return 0x0135; // small j with circumflex
  case 0xC36F:
    return 0x00F4; // small o with circumflex accent
  case 0xC373:
    return 0x015D; // small s with circumflex
  case 0xC375:
    return 0x00FB; // small u with circumflex
  case 0xC377:
    return 0x0175; // small w with circumflex
  case 0xC379:
    return 0x0177; // small y with circumflex
  case 0xC37A:
    return 0x1E91; // small z with circumflex

  // 4/4 tilde
  case 0xC441:
    return 0x00C3; // CAPITAL A WITH TILDE
  case 0xC445:
    return 0x1EBC; // CAPITAL E WITH TILDE
  case 0xC449:
    return 0x0128; // CAPITAL I WITH TILDE
  case 0xC44E:
    return 0x00D1; // CAPITAL N WITH TILDE
  case 0xC44F:
    return 0x00D5; // CAPITAL O WITH TILDE
  case 0xC455:
    return 0x0168; // CAPITAL U WITH TILDE
  case 0xC456:
    return 0x1E7C; // CAPITAL V WITH TILDE
  case 0xC459:
    return 0x1EF8; // CAPITAL Y WITH TILDE
  case 0xC461:
    return 0x00E3; // small a with tilde
  case 0xC465:
    return 0x1EBD; // small e with tilde
  case 0xC469:
    return 0x0129; // small i with tilde
  case 0xC46E:
    return 0x00F1; // small n with tilde
  case 0xC46F:
    return 0x00F5; // small o with tilde
  case 0xC475:
    return 0x0169; // small u with tilde
  case 0xC476:
    return 0x1E7D; // small v with tilde
  case 0xC479:
    return 0x1EF9; // small y with tilde
  case 0xC4E1: // is this an error
    return 0x01E2; // CAPITAL AE WITH MACRON
  case 0xC4F1:
    return 0x01E3; // small ae with macron

  // 4/5 macron
  case 0xC541:
    return 0x0100; // CAPITAL A WITH MACRON
  case 0xC545:
    return 0x0112; // CAPITAL E WITH MACRON
  case 0xC547:
    return 0x1E20; // CAPITAL G WITH MACRON
  case 0xC549:
    return 0x012A; // CAPITAL I WITH MACRON
  case 0xC54F:
    return 0x014C; // CAPITAL O WITH MACRON
  case 0xC555:
    return 0x016A; // CAPITAL U WITH MACRON
  case 0xC559:
    return 0x0232; // CAPITAL LETTER Y WITH MACRON
  case 0xC561:
    return 0x0101; // small a with macron
  case 0xC565:
    return 0x0113; // small e with macron
  case 0xC567:
    return 0x1E21; // small g with macron
  case 0xC569:
    return 0x012B; // small i with macron
  case 0xC56F:
    return 0x014D; // small o with macron
  case 0xC575:
    return 0x016B; // small u with macron
  case 0xC579:
    return 0x0233; // Small LETTER Y WITH MACRON
  case 0xC5E1:
    return 0x01E2; // CAPITAL AE WITH MACRON
  case 0xC5F1:
    return 0x01E3; // small ae with macron

  // 4/6 breve
  case 0xC641:
    return 0x0102; // CAPITAL A WITH BREVE
  case 0xC645:
    return 0x0114; // CAPITAL E WITH BREVE
  case 0xC647:
    return 0x011E; // CAPITAL G WITH BREVE
  case 0xC649:
    return 0x012C; // CAPITAL I WITH BREVE
  case 0xC64F:
    return 0x014E; // CAPITAL O WITH BREVE
  case 0xC655:
    return 0x016C; // CAPITAL U WITH BREVE
  case 0xC661:
    return 0x0103; // small a with breve
  case 0xC665:
    return 0x0115; // small e with breve
  case 0xC667:
    return 0x011F; // small g with breve
  case 0xC669:
    return 0x012D; // small i with breve
  case 0xC66F:
    return 0x014F; // small o with breve
  case 0xC675:
    return 0x016D; // small u with breve

  // 4/7 dot above
  case 0xC741:
    return 0x0226; // CAPITAL LETTER A WITH DOT ABOVE
  case 0xC742:
    return 0x1E02; // CAPITAL B WITH DOT ABOVE
  case 0xC743:
    return 0x010A; // CAPITAL C WITH DOT ABOVE
  case 0xC744:
    return 0x1E0A; // CAPITAL D WITH DOT ABOVE
  case 0xC745:
    return 0x0116; // CAPITAL E WITH DOT ABOVE
  case 0xC746:
    return 0x1E1E; // CAPITAL F WITH DOT ABOVE
  case 0xC747:
    return 0x0120; // CAPITAL G WITH DOT ABOVE
  case 0xC748:
    return 0x1E22; // CAPITAL H WITH DOT ABOVE
  case 0xC749:
    return 0x0130; // CAPITAL I WITH DOT ABOVE
  case 0xC74D:
    return 0x1E40; // CAPITAL M WITH DOT ABOVE
  case 0xC74E:
    return 0x1E44; // CAPITAL N WITH DOT ABOVE
  case 0xC74F:
    return 0x022E; // CAPITAL LETTER O WITH DOT ABOVE
  case 0xC750:
    return 0x1E56; // CAPITAL P WITH DOT ABOVE
  case 0xC752:
    return 0x1E58; // CAPITAL R WITH DOT ABOVE
  case 0xC753:
    return 0x1E60; // CAPITAL S WITH DOT ABOVE
  case 0xC754:
    return 0x1E6A; // CAPITAL T WITH DOT ABOVE
  case 0xC757:
    return 0x1E86; // CAPITAL W WITH DOT ABOVE
  case 0xC758:
    return 0x1E8A; // CAPITAL X WITH DOT ABOVE
  case 0xC759:
    return 0x1E8E; // CAPITAL Y WITH DOT ABOVE
  case 0xC75A:
    return 0x017B; // CAPITAL Z WITH DOT ABOVE
  case 0xC761:
    return 0x0227; // small LETTER A WITH DOT ABOVE
  case 0xC762:
    return 0x1E03; // small b with dot above
  case 0xC763:
    return 0x010B; // small c with dot above
  case 0xC764:
    return 0x1E0B; // small d with dot above
  case 0xC765:
    return 0x0117; // small e with dot above
  case 0xC766:
    return 0x1E1F; // small f with dot above
  case 0xC767:
    return 0x0121; // small g with dot above
  case 0xC768:
    return 0x1E23; // small h with dot above
  case 0xC76D:
    return 0x1E41; // small m with dot above
  case 0xC76E:
    return 0x1E45; // small n with dot above
  case 0xC76F:
    return 0x022F; // SMALL LETTER O WITH DOT ABOVE
  case 0xC770:
    return 0x1E57; // small p with dot above
  case 0xC772:
    return 0x1E59; // small r with dot above
  case 0xC773:
    return 0x1E61; // small s with dot above
  case 0xC774:
    return 0x1E6B; // small t with dot above
  case 0xC777:
    return 0x1E87; // small w with dot above
  case 0xC778:
    return 0x1E8B; // small x with dot above
  case 0xC779:
    return 0x1E8F; // small y with dot above
  case 0xC77A:
    return 0x017C; // small z with dot above

  // 4/8 trema, diaresis
  case 0xC820:
    return 0x00A8; // diaeresis
  case 0xC841:
    return 0x00C4; // CAPITAL A WITH DIAERESIS
  case 0xC845:
    return 0x00CB; // CAPITAL E WITH DIAERESIS
  case 0xC848:
    return 0x1E26; // CAPITAL H WITH DIAERESIS
  case 0xC849:
    return 0x00CF; // CAPITAL I WITH DIAERESIS
  case 0xC84F:
    return 0x00D6; // CAPITAL O WITH DIAERESIS
  case 0xC855:
    return 0x00DC; // CAPITAL U WITH DIAERESIS
  case 0xC857:
    return 0x1E84; // CAPITAL W WITH DIAERESIS
  case 0xC858:
    return 0x1E8C; // CAPITAL X WITH DIAERESIS
  case 0xC859:
    return 0x0178; // CAPITAL Y WITH DIAERESIS
  case 0xC861:
    return 0x00E4; // small a with diaeresis
  case 0xC865:
    return 0x00EB; // small e with diaeresis
  case 0xC868:
    return 0x1E27; // small h with diaeresis
  case 0xC869:
    return 0x00EF; // small i with diaeresis
  case 0xC86F:
    return 0x00F6; // small o with diaeresis
  case 0xC874:
    return 0x1E97; // small t with diaeresis
  case 0xC875:
    return 0x00FC; // small u with diaeresis
  case 0xC877:
    return 0x1E85; // small w with diaeresis
  case 0xC878:
    return 0x1E8D; // small x with diaeresis
  case 0xC879:
    return 0x00FF; // small y with diaeresis

  // 4/9 umlaut
  case 0xC920:
    return 0x00A8; // [diaeresis]

  // 4/10 circle above
  case 0xCA41:
    return 0x00C5; // CAPITAL A WITH RING ABOVE
  case 0xCA55: // was CAAD
    return 0x016E; // CAPITAL U WITH RING ABOVE
  case 0xCA61:
    return 0x00E5; // small a with ring above
  case 0xCA75:
    return 0x016F; // small u with ring above
  case 0xCA77:
    return 0x1E98; // small w with ring above
  case 0xCA79:
    return 0x1E99; // small y with ring above

  // 4/11 high comma off centre

  // 4/12 inverted high comma centred

  // 4/13 double acute accent
  case 0xCD4F:
    return 0x0150; // CAPITAL O WITH DOUBLE ACUTE
  case 0xCD55:
    return 0x0170; // CAPITAL U WITH DOUBLE ACUTE
  case 0xCD6F:
    return 0x0151; // small o with double acute
  case 0xCD75:
    return 0x0171; // small u with double acute

  // 4/14 horn
  case 0xCE4F: // was 0xCE54
    return 0x01A0; // LATIN CAPITAL LETTER O WITH HORN
  case 0xCE55:
    return 0x01AF; // LATIN CAPITAL LETTER U WITH HORN
  case 0xCE6F: // was 0xCE74
    return 0x01A1; // latin small letter o with horn
  case 0xCE75:
    return 0x01B0; // latin small letter u with horn

  // 4/15 caron (hacek)
  case 0xCF41:
    return 0x01CD; // CAPITAL A WITH CARON
  case 0xCF43:
    return 0x010C; // CAPITAL C WITH CARON
  case 0xCF44:
    return 0x010E; // CAPITAL D WITH CARON
  case 0xCF45:
    return 0x011A; // CAPITAL E WITH CARON
  case 0xCF47:
    return 0x01E6; // CAPITAL G WITH CARON
  case 0xCF48:
    return 0x021E; // CAPITAL LETTER H WITH CARON
  case 0xCF49:
    return 0x01CF; // CAPITAL I WITH CARON
  case 0xCF4B:
    return 0x01E8; // CAPITAL K WITH CARON
  case 0xCF4C:
    return 0x013D; // CAPITAL L WITH CARON
  case 0xCF4E:
    return 0x0147; // CAPITAL N WITH CARON
  case 0xCF4F:
    return 0x01D1; // CAPITAL O WITH CARON
  case 0xCF52:
    return 0x0158; // CAPITAL R WITH CARON
  case 0xCF53:
    return 0x0160; // CAPITAL S WITH CARON
  case 0xCF54:
    return 0x0164; // CAPITAL T WITH CARON
  case 0xCF55:
    return 0x01D3; // CAPITAL U WITH CARON
  case 0xCF5A:
    return 0x017D; // CAPITAL Z WITH CARON
  case 0xCF61:
    return 0x01CE; // small a with caron
  case 0xCF63:
    return 0x010D; // small c with caron
  case 0xCF64:
    return 0x010F; // small d with caron
  case 0xCF65:
    return 0x011B; // small e with caron
  case 0xCF67:
    return 0x01E7; // small g with caron
  case 0xCF68:
    return 0x021F; // small LETTER H WITH CARON
  case 0xCF69:
    return 0x01D0; // small i with caron
  case 0xCF6A:
    return 0x01F0; // small j with caron
  case 0xCF6B:
    return 0x01E9; // small k with caron
  case 0xCF6C:
    return 0x013E; // small l with caron
  case 0xCF6E:
    return 0x0148; // small n with caron
  case 0xCF6F:
    return 0x01D2; // small o with caron
  case 0xCF72:
    return 0x0159; // small r with caron
  case 0xCF73:
    return 0x0161; // small s with caron
  case 0xCF74:
    return 0x0165; // small t with caron
  case 0xCF75:
    return 0x01D4; // small u with caron
  case 0xCF7A:
    return 0x017E; // small z with caron

  // 5/0 cedilla
  case 0xD020:
    return 0x00B8; // cedilla
  case 0xD043:
    return 0x00C7; // CAPITAL C WITH CEDILLA
  case 0xD044:
    return 0x1E10; // CAPITAL D WITH CEDILLA
  case 0xD045:
    return 0x0228; // CAPITAL LETTER E WITH CEDILLA
  case 0xD047:
    return 0x0122; // CAPITAL G WITH CEDILLA
  case 0xD048:
    return 0x1E28; // CAPITAL H WITH CEDILLA
  case 0xD04B:
    return 0x0136; // CAPITAL K WITH CEDILLA
  case 0xD04C:
    return 0x013B; // CAPITAL L WITH CEDILLA
  case 0xD04E:
    return 0x0145; // CAPITAL N WITH CEDILLA
  case 0xD052:
    return 0x0156; // CAPITAL R WITH CEDILLA
  case 0xD053:
    return 0x015E; // CAPITAL S WITH CEDILLA
  case 0xD054:
    return 0x0162; // CAPITAL T WITH CEDILLA
  case 0xD063:
    return 0x00E7; // small c with cedilla
  case 0xD064:
    return 0x1E11; // small d with cedilla
  case 0xD065:
    return 0x0229; // small LETTER E WITH CEDILLA
  case 0xD067:
    return 0x0123; // small g with cedilla
  case 0xD068:
    return 0x1E29; // small h with cedilla
  case 0xD06B:
    return 0x0137; // small k with cedilla
  case 0xD06C:
    return 0x013C; // small l with cedilla
  case 0xD06E:
    return 0x0146; // small n with cedilla
  case 0xD072:
    return 0x0157; // small r with cedilla
  case 0xD073:
    return 0x015F; // small s with cedilla
  case 0xD074:
    return 0x0163; // small t with cedilla

  // 5/1 rude

  // 5/2 hook to left
  case 0xD253:
    return 0x0218; // CAPITAL LETTER S WITH COMMA BELOW
  case 0xD254:
    return 0x021A; // CAPITAL LETTER T WITH COMMA BELOW
  case 0xD273:
    return 0x0219; // Small LETTER S WITH COMMA BELOW
  case 0xD274:
    return 0x021B; // Small LETTER T WITH COMMA BELOW

  // 5/3 ogonek (hook to right)
  case 0xD320:
    return 0x02DB; // ogonek
  case 0xD341:
    return 0x0104; // CAPITAL A WITH OGONEK
  case 0xD345:
    return 0x0118; // CAPITAL E WITH OGONEK
  case 0xD349:
    return 0x012E; // CAPITAL I WITH OGONEK
  case 0xD34F:
    return 0x01EA; // CAPITAL O WITH OGONEK
  case 0xD355:
    return 0x0172; // CAPITAL U WITH OGONEK
  case 0xD361:
    return 0x0105; // small a with ogonek
  case 0xD365:
    return 0x0119; // small e with ogonek
  case 0xD369:
    return 0x012F; // small i with ogonek
  case 0xD36F:
    return 0x01EB; // small o with ogonek
  case 0xD375:
    return 0x0173; // small u with ogonek

  // 5/4 circle below
  case 0xD441:
    return 0x1E00; // CAPITAL A WITH RING BELOW
  case 0xD461:
    return 0x1E01; // small a with ring below
  case 0xD548:
    return 0x1E2A; // CAPITAL LETTER H WITH BREVE BELOW
  case 0xD568:
    return 0x1E2B; // small LETTER H WITH BREVE BELOW

  // 5/6 dot below
  case 0xD641:
    return 0x1EA0; // CAPITAL A WITH DOT BELOW
  case 0xD642:
    return 0x1E04; // CAPITAL B WITH DOT BELOW
  case 0xD644:
    return 0x1E0C; // CAPITAL D WITH DOT BELOW
  case 0xD645:
    return 0x1EB8; // CAPITAL E WITH DOT BELOW
  case 0xD648:
    return 0x1E24; // CAPITAL H WITH DOT BELOW
  case 0xD649:
    return 0x1ECA; // CAPITAL I WITH DOT BELOW
  case 0xD64B:
    return 0x1E32; // CAPITAL K WITH DOT BELOW
  case 0xD64C:
    return 0x1E36; // CAPITAL L WITH DOT BELOW
  case 0xD64D:
    return 0x1E42; // CAPITAL M WITH DOT BELOW
  case 0xD64E:
    return 0x1E46; // CAPITAL N WITH DOT BELOW
  case 0xD64F:
    return 0x1ECC; // CAPITAL O WITH DOT BELOW
  case 0xD652:
    return 0x1E5A; // CAPITAL R WITH DOT BELOW
  case 0xD653:
    return 0x1E62; // CAPITAL S WITH DOT BELOW
  case 0xD654:
    return 0x1E6C; // CAPITAL T WITH DOT BELOW
  case 0xD655:
    return 0x1EE4; // CAPITAL U WITH DOT BELOW
  case 0xD656:
    return 0x1E7E; // CAPITAL V WITH DOT BELOW
  case 0xD657:
    return 0x1E88; // CAPITAL W WITH DOT BELOW
  case 0xD659:
    return 0x1EF4; // CAPITAL Y WITH DOT BELOW
  case 0xD65A:
    return 0x1E92; // CAPITAL Z WITH DOT BELOW
  case 0xD661:
    return 0x1EA1; // small a with dot below
  case 0xD662:
    return 0x1E05; // small b with dot below
  case 0xD664:
    return 0x1E0D; // small d with dot below
  case 0xD665:
    return 0x1EB9; // small e with dot below
  case 0xD668:
    return 0x1E25; // small h with dot below
  case 0xD669:
    return 0x1ECB; // small i with dot below
  case 0xD66B:
    return 0x1E33; // small k with dot below
  case 0xD66C:
    return 0x1E37; // small l with dot below
  case 0xD66D:
    return 0x1E43; // small m with dot below
  case 0xD66E:
    return 0x1E47; // small n with dot below
  case 0xD66F:
    return 0x1ECD; // small o with dot below
  case 0xD672:
    return 0x1E5B; // small r with dot below
  case 0xD673:
    return 0x1E63; // small s with dot below
  case 0xD674:
    return 0x1E6D; // small t with dot below
  case 0xD675:
    return 0x1EE5; // small u with dot below
  case 0xD676:
    return 0x1E7F; // small v with dot below
  case 0xD677:
    return 0x1E89; // small w with dot below
  case 0xD679:
    return 0x1EF5; // small y with dot below
  case 0xD67A:
    return 0x1E93; // small z with dot below

  // 5/7 double dot below
  case 0xD755:
    return 0x1E72; // CAPITAL U WITH DIAERESIS BELOW
  case 0xD775:
    return 0x1E73; // small u with diaeresis below

  // 5/8 underline
  case 0xD820:
    return 0x005F; // underline

  // 5/9 double underline
  case 0xD920:
    return 0x2017; // double underline

  // 5/10 small low vertical bar
  case 0xDA20:
    return 0x02CC; //

  case 0xDB44:
    return 0x1E12; // CAPITAL LETTER D WITH CIRCUMFLEX BELOW
  case 0xDB45:
    return 0x1E18; // CAPITAL LETTER E WITH CIRCUMFLEX BELOW
  case 0xDB4C:
    return 0x1E3C; // CAPITAL LETTER L WITH CIRCUMFLEX BELOW
  case 0xDB4E:
    return 0x1E4A; // CAPITAL LETTER N WITH CIRCUMFLEX BELOW
  case 0xDB54:
    return 0x1E70; // CAPITAL LETTER T WITH CIRCUMFLEX BELOW
  case 0xDB55:
    return 0x1E76; // CAPITAL LETTER U WITH CIRCUMFLEX BELOW
  case 0xDB64:
    return 0x1E13; // SMALL LETTER D WITH CIRCUMFLEX BELOW
  case 0xDB65:
    return 0x1E19; // SMALL LETTER E WITH CIRCUMFLEX BELOW
  case 0xDB6C:
    return 0x1E3D; // Small LETTER L WITH CIRCUMFLEX BELOW
  case 0xDB6E:
    return 0x1E4B; // SMALL LETTER N WITH CIRCUMFLEX BELOW
  case 0xDB74:
    return 0x1E71; // SMALL LETTER T WITH CIRCUMFLEX BELOW
  case 0xDB75:
    return 0x1E77; // SMALL LETTER U WITH CIRCUMFLEX BELOW

  // 5/5 half circle below
  case 0xF948:
    return 0x1E2A; // CAPITAL H WITH BREVE BELOW
  case 0xF968:
    return 0x1E2B; // small h with breve below

  // 5/11 circumflex below

  // 5/12 (this position shall not be used)

  // 5/13 left half of ligature sign and of double tilde

  // 5/14 right half of ligature sign

  // 5/15 right half of double tilde

  case 0xC0C341:
    return 0x1EA8; // CAPITAL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE
  case 0xC0C345:
    return 0x1EC2; // CAPITAL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE
  case 0xC0C34F:
    return 0x1ED4; // CAPITAL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE
  case 0xC0C361:
    return 0x1EA9; // SMALL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE
  case 0xC0C365:
    return 0x1EC3; // SMALL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE
  case 0xC0C36F:
    return 0x1ED5; // SMALL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE
  case 0xC0C641:
    return 0x1EB2; // CAPITAL LETTER A WITH BREVE AND HOOK ABOVE
  case 0xC0C661:
    return 0x1EB3; // SMALL LETTER A WITH BREVE AND HOOK ABOVE
  case 0xC0CE4F:
    return 0x1EDE; // CAPITAL LETTER O WITH HORN AND HOOK ABOVE
  case 0xC0CE55:
    return 0x1EEC; // CAPITAL LETTER U WITH HORN AND HOOK ABOVE
  case 0xC0CE6F:
    return 0x1EDF; // SMALL LETTER O WITH HORN AND HOOK ABOVE
  case 0xC0CE75:
    return 0x1EED; // SMALL LETTER U WITH HORN AND HOOK ABOVE

  case 0xC1C341:
    return 0x1EA6; // CAPITAL LETTER A WITH CIRCUMFLEX AND GRAVE
  case 0xC1C345:
    return 0x1EC0; // CAPITAL LETTER E WITH CIRCUMFLEX AND GRAVE
  case 0xC1C34F:
    return 0x1ED2; // CAPITAL LETTER O WITH CIRCUMFLEX AND GRAVE
  case 0xC1C361:
    return 0x1EA7; // SMALL LETTER A WITH CIRCUMFLEX AND GRAVE
  case 0xC1C365:
    return 0x1EC1; // SMALL LETTER E WITH CIRCUMFLEX AND GRAVE
  case 0xC1C36F:
    return 0x1ED3; // SMALL LETTER O WITH CIRCUMFLEX AND GRAVE
  case 0xC1C545:
    return 0x1E14; // CAPITAL LETTER E WITH MACRON AND GRAVE
  case 0xC1C54F:
    return 0x1E50; // CAPITAL LETTER O WITH MACRON AND GRAVE
  case 0xC1C565:
    return 0x1E15; // SMALL LETTER E WITH MACRON AND GRAVE
  case 0xC1C56F:
    return 0x1E51; // SMALL LETTER O WITH MACRON AND GRAVE
  case 0xC1C641:
    return 0x1EB0; // CAPITAL LETTER A WITH BREVE AND GRAVE
  case 0xC1C661:
    return 0x1EB1; // SMALL LETTER A WITH BREVE AND GRAVE
  case 0xC1C855:
    return 0x01DB; // Capital Letter U with Diaeresis and GRAVE
  case 0xC1C875:
    return 0x01DC; // Small Letter U with Diaeresis and GRAVE
  case 0xC1CE4F:
    return 0x1EDC; // CAPITAL LETTER O WITH HORN AND GRAVE
  case 0xC1CE55:
    return 0x1EEA; // CAPITAL LETTER U WITH HORN AND GRAVE
  case 0xC1CE6F:
    return 0x1EDD; // SMALL LETTER O WITH HORN AND GRAVE
  case 0xC1CE75:
    return 0x1EEB; // SMALL LETTER U WITH HORN AND GRAVE

  case 0xC2C341:
    return 0x1EA4; // CAPITAL LETTER A WITH CIRCUMFLEX AND ACUTE
  case 0xC2C345:
    return 0x1EBE; // CAPITAL LETTER E WITH CIRCUMFLEX AND ACUTE
  case 0xC2C34F:
    return 0x1ED0; // CAPITAL LETTER O WITH CIRCUMFLEX AND ACUTE
  case 0xC2C361:
    return 0x1EA5; // SMALL LETTER A WITH CIRCUMFLEX AND ACUTE
  case 0xC2C365:
    return 0x1EBF; // SMALL LETTER E WITH CIRCUMFLEX AND ACUTE
  case 0xC2C36F:
    return 0x1ED1; // SMALL LETTER O WITH CIRCUMFLEX AND ACUTE
  case 0xC2C44F:
    return 0x1E4C; // CAPITAL LETTER O WITH TILDE AND ACUTE
  case 0xC2C455:
    return 0x1E78; // CAPITAL LETTER U WITH TILDE AND ACUTE
  case 0xC2C46F:
    return 0x1E4D; // SMALL LETTER O WITH TILDE AND ACUTE
  case 0xC2C475:
    return 0x1E79; // SMALL LETTER U WITH TILDE AND ACUTE
  case 0xC2C545:
    return 0x1E16; // CAPITAL LETTER E WITH MACRON AND ACUTE
  case 0xC2C565:
    return 0x1E17; // SMALL LETTER E WITH MACRON AND ACUTE
  case 0xC2C54F:
    return 0x1E52; // CAPITAL LETTER O WITH MACRON AND ACUTE
  case 0xC2C56F:
    return 0x1E53; // SMALL LETTER O WITH MACRON AND ACUTE
  case 0xC2C641:
    return 0x1EAE; // CAPITAL LETTER A WITH BREVE AND ACUTE
  case 0xC2C661:
    return 0x1EAF; // SMALL LETTER A WITH BREVE AND ACUTE
  case 0xC2C849:
    return 0x1E2E; // CAPITAL LETTER I WITH DIAERESIS AND ACUTE
  case 0xC2C855:
    return 0x01D7; // Capital Letter U with Diaeresis and ACUTE
  case 0xC2C869:
    return 0x1E2F; // Small LETTER I WITH DIAERESIS AND ACUTE
  case 0xC2C875:
    return 0x01D8; // Small Letter U with Diaeresis and ACUTE
  case 0xC2CA41:
    return 0x01FA; // CAPITAL LETTER A WITH RING ABOVE AND ACUTE
  case 0xC2CA61:
    return 0x01FB; // SMALL LETTER A WITH RING ABOVE AND ACUTE
  case 0xC2CE4F:
    return 0x1EDA; // CAPITAL LETTER O WITH HORN AND ACUTE
  case 0xC2CE55:
    return 0x1EE8; // CAPITAL LETTER U WITH HORN AND ACUTE
  case 0xC2CE6F:
    return 0x1EDB; // SMALL LETTER O WITH HORN AND ACUTE
  case 0xC2CE75:
    return 0x1EE9; // small LETTER U WITH HORN AND ACUTE
  case 0xC2D043:
    return 0x1E08; // CAPITAL LETTER C WITH CEDILLA AND ACUTE
  case 0xC2D063:
    return 0x1E09; // Small LETTER C WITH CEDILLA AND ACUTE

  case 0xC3D641:
    return 0x1EAC; // CAPITAL LETTER A WITH CIRCUMFLEX AND DOT BELOW
  case 0xC3D645:
    return 0x1EC6; // CAPITAL LETTER E WITH CIRCUMFLEX AND DOT BELOW
  case 0xC3D64F:
    return 0x1ED8; // CAPITAL LETTER O WITH CIRCUMFLEX AND DOT BELOW
  case 0xC3D661:
    return 0x1EAD; // SMALL LETTER A WITH CIRCUMFLEX AND DOT BELOW
  case 0xC3D665:
    return 0x1EC7; // SMALL LETTER E WITH CIRCUMFLEX AND DOT BELOW
  case 0xC3D66F:
    return 0x1ED9; // SMALL LETTER O WITH CIRCUMFLEX AND DOT BELOW

  case 0xC4C341:
    return 0x1EAA; // CAPITAL LETTER A WITH CIRCUMFLEX AND TILDE
  case 0xC4C345:
    return 0x1EC4; // CAPITAL LETTER E WITH CIRCUMFLEX AND TILDE
  case 0xC4C34F:
    return 0x1ED6; // CAPITAL LETTER O WITH CIRCUMFLEX AND TILDE
  case 0xC4C361:
    return 0x1EAB; // SMALL LETTER A WITH CIRCUMFLEX AND TILDE
  case 0xC4C365:
    return 0x1EC5; // SMALL LETTER E WITH CIRCUMFLEX AND TILDE
  case 0xC4C36F:
    return 0x1ED7; // SMALL LETTER O WITH CIRCUMFLEX AND TILDE
  case 0xC4C641:
    return 0x1EB4; // CAPITAL LETTER A WITH BREVE AND TILDE
  case 0xC4C661:
    return 0x1EB5; // SMALL LETTER A WITH BREVE AND TILDE
  case 0xC4CE4F:
    return 0x1EE0; // CAPITAL LETTER O WITH HORN AND TILDE
  case 0xC4CE55:
    return 0x1EEE; // CAPITAL LETTER U WITH HORN AND TILDE
  case 0xC4CE6F:
    return 0x1EE1; // SMALL LETTER O WITH HORN AND TILDE
  case 0xC4CE75:
    return 0x1EEF; // SMALL LETTER U WITH HORN AND TILDE

  case 0xC5C44F:
    return 0x022C; // CAPITAL LETTER O WITH TILDE AND MACRON
  case 0xC5C741:
    return 0x01E0; // CAPITAL LETTER A WITH DOT ABOVE AND MACRON
  case 0xC5C74F:
    return 0x0230; // CAPITAL LETTER O WITH DOT ABOVE AND MACRON
  case 0xC5C761:
    return 0x01E1; // SMALL LETTER A WITH DOT ABOVE AND MACRON
  case 0xC5C76F:
    return 0x0231; // SMALL LETTER O WITH DOT ABOVE AND MACRON
  case 0xC5C841:
    return 0x01DE; // CAPITAL LETTER A WITH DIAERESIS AND MACRON
  case 0xC5C84F:
    return 0x022A; // CAPITAL LETTER O WITH DIAERESIS AND MACRON
  case 0xC5C855:
    return 0x01D5; // Capital Letter U with Diaeresis and Macron
  case 0xC5C861:
    return 0x01DF; // Small LETTER A WITH DIAERESIS AND MACRON
  case 0xC5C875:
    return 0x01D6; // Small Letter U with Diaeresis and Macron
  case 0xC5D34F:
    return 0x01EC; // CAPITAL LETTER O WITH OGONEK AND MACRON
  case 0xC5D36F:
    return 0x01ED; // SMALL LETTER O WITH OGONEK AND MACRON
  case 0xC5C46F:
    return 0x022D; // SMALL LETTER O WITH TILDE AND MACRON
  case 0xC5D64C:
    return 0x1E38; // CAPITAL LETTER L WITH DOT BELOW AND MACRON
  case 0xC5D652:
    return 0x1E5C; // CAPITAL LETTER R WITH DOT BELOW AND MACRON
  case 0xC5D66C:
    return 0x1E39; // Small LETTER L WITH DOT BELOW AND MACRON
  case 0xC5D672:
    return 0x1E5D; // SMALL LETTER R WITH DOT BELOW AND MACRON
  case 0xC5C86F:
    return 0x022B; // small LETTER O WITH DIAERESIS AND MACRON

  case 0xC6D045:
    return 0x1E1C; // CAPITAL LETTER E WITH CEDILLA AND BREVE
  case 0xC6D065:
    return 0x1E1D; // Small LETTER E WITH CEDILLA AND BREVE
  case 0xC6D641:
    return 0x1EB6; // CAPITAL LETTER A WITH BREVE AND DOT BELOW
  case 0xC6D661:
    return 0x1EB7; // SMALL LETTER A WITH BREVE AND DOT BELOW

  case 0xC7C253:
    return 0x1E64; // CAPITAL LETTER S WITH ACUTE AND DOT ABOVE
  case 0xC7C273:
    return 0x1E65; // SMALL LETTER S WITH ACUTE AND DOT ABOVE
  case 0xC7CF53:
    return 0x1E66; // CAPITAL LETTER S WITH CARON AND DOT ABOVE
  case 0xC7CF73:
    return 0x1E67; // SMALL LETTER S WITH CARON AND DOT ABOVE
  case 0xC7D653:
    return 0x1E68; // CAPITAL LETTER S WITH DOT BELOW AND DOT ABOVE
  case 0xC7D673:
    return 0x1E69; // SMALL LETTER S WITH DOT BELOW AND DOT ABOVE

  case 0xC8C44F:
    return 0x1E4E; // CAPITAL LETTER O WITH TILDE AND DIAERESIS
  case 0xC8C46F:
    return 0x1E4F; // SMALL LETTER O WITH TILDE AND DIAERESIS
  case 0xC8C555:
    return 0x1E7A; // CAPITAL LETTER U WITH MACRON AND DIAERESIS
  case 0xC8C575:
    return 0x1E7B; // SMALL LETTER U WITH MACRON AND DIAERESIS

  case 0xCFC855:
    return 0x01D9; // Capital Letter U with Diaeresis and CARON
  case 0xCFC875:
    return 0x01DA; // Small Letter U with Diaeresis and CARON

  case 0xD6CE4F:
    return 0x1EE2; // CAPITAL LETTER O WITH HORN AND DOT BELOW
  case 0xD6CE55:
    return 0x1EF0; // CAPITAL LETTER U WITH HORN AND DOT BELOW
  case 0xD6CE6F:
    return 0x1EE3; // SMALL LETTER O WITH HORN AND DOT BELOW
  case 0xD6CE75:
    return 0x1EF1; // SMALL LETTER U WITH HORN AND DOT BELOW

  default:
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
    myDebug() << "no match for" << hex << c;
#else
    myDebug() << "no match for" << Qt::hex << c;
#endif
    return 0;
  }
}
