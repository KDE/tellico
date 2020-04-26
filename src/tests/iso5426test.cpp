/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include "iso5426test.h"
#include "../utils/iso5426converter.h"

#include <QTest>

QTEST_APPLESS_MAIN( Iso5426Test )

void Iso5426Test::testStrings() {
  QFETCH(QString, output);
  QFETCH(QByteArray, input);

  QCOMPARE(Tellico::Iso5426Converter::toUtf8(input), output);
}

void Iso5426Test::testStrings_data() {
  QTest::addColumn<QString>("output");
  QTest::addColumn<QByteArray>("input");

  QTest::newRow("lowercase") << QStringLiteral("abcdefghijklmnopqrstuvwxyz") << QByteArray("abcdefghijklmnopqrstuvwxyz");
  QTest::newRow("uppercase") << QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ") << QByteArray("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  QTest::newRow("numbers") << QStringLiteral("1234567890") << QByteArray("1234567890");
  QTest::newRow("symbols") << QStringLiteral("`-=[]\\;',./~!@#$%^&*()_+{}|:\"<>?") << QByteArray("`-=[]\\;',./~!@#$%^&*()_+{}|:\"<>?");
  // have to string concatenation since the hex looks for any length of valid characters
  QTest::newRow("accents") << QStringLiteral("abcdefgäöüậ") << QByteArray("abcdefg\xc8" "a\xc8" "o\xc8" "u\xc3\xd6" "a");
}

void Iso5426Test::testCharacters() {
  QFETCH(QString, output);
  QFETCH(QByteArray, input);

  QCOMPARE(Tellico::Iso5426Converter::toUtf8(input), output);
}

// https://bitbucket.org/tiran/smc.bibencodings/src/default/smc/bibencodings/iso5426.py
void Iso5426Test::testCharacters_data() {
  QTest::addColumn<QString>("output");
  QTest::addColumn<QByteArray>("input");

  QTest::newRow("0020") << QStringLiteral("\u0020") << QByteArray(" "); // SPACE
  QTest::newRow("0021") << QStringLiteral("\u0021") << QByteArray("!"); // EXCLAMATION MARK
  QTest::newRow("0022") << QStringLiteral("\u0022") << QByteArray("\""); // QUOTATION MARK
  QTest::newRow("0023") << QStringLiteral("\u0023") << QByteArray("#"); // NUMBER SIGN
  QTest::newRow("0025") << QStringLiteral("\u0025") << QByteArray("%"); // PERCENT SIGN
  QTest::newRow("0026") << QStringLiteral("\u0026") << QByteArray("&"); // AMPERSAND
  QTest::newRow("0027") << QStringLiteral("\u0027") << QByteArray("'"); // APOSTROPHE
  QTest::newRow("0028") << QStringLiteral("\u0028") << QByteArray("("); // LEFT PARENTHESIS
  QTest::newRow("0029") << QStringLiteral("\u0029") << QByteArray(")"); // RIGHT PARENTHESIS
  QTest::newRow("002a") << QStringLiteral("\u002a") << QByteArray("*"); // ASTERISK
  QTest::newRow("002b") << QStringLiteral("\u002b") << QByteArray("+"); // PLUS SIGN
  QTest::newRow("002c") << QStringLiteral("\u002c") << QByteArray(","); // COMMA
  QTest::newRow("002d") << QStringLiteral("\u002d") << QByteArray("-"); // HYPHEN-MINUS
  QTest::newRow("002e") << QStringLiteral("\u002e") << QByteArray("."); // FULL STOP
  QTest::newRow("002f") << QStringLiteral("\u002f") << QByteArray("/"); // SOLIDUS
  QTest::newRow("0030") << QStringLiteral("\u0030") << QByteArray("0"); // DIGIT ZERO
  QTest::newRow("0031") << QStringLiteral("\u0031") << QByteArray("1"); // DIGIT ONE
  QTest::newRow("0032") << QStringLiteral("\u0032") << QByteArray("2"); // DIGIT TWO
  QTest::newRow("0033") << QStringLiteral("\u0033") << QByteArray("3"); // DIGIT THREE
  QTest::newRow("0034") << QStringLiteral("\u0034") << QByteArray("4"); // DIGIT FOUR
  QTest::newRow("0035") << QStringLiteral("\u0035") << QByteArray("5"); // DIGIT FIVE
  QTest::newRow("0036") << QStringLiteral("\u0036") << QByteArray("6"); // DIGIT SIX
  QTest::newRow("0037") << QStringLiteral("\u0037") << QByteArray("7"); // DIGIT SEVEN
  QTest::newRow("0038") << QStringLiteral("\u0038") << QByteArray("8"); // DIGIT EIGHT
  QTest::newRow("0039") << QStringLiteral("\u0039") << QByteArray("9"); // DIGIT NINE
  QTest::newRow("003a") << QStringLiteral("\u003a") << QByteArray(":"); // COLON
  QTest::newRow("003b") << QStringLiteral("\u003b") << QByteArray(";"); // SEMICOLON
  QTest::newRow("003c") << QStringLiteral("\u003c") << QByteArray("<"); // LESS-THAN SIGN
  QTest::newRow("003d") << QStringLiteral("\u003d") << QByteArray("="); // EQUALS SIGN
  QTest::newRow("003e") << QStringLiteral("\u003e") << QByteArray(">"); // GREATER-THAN SIGN
  QTest::newRow("003f") << QStringLiteral("\u003f") << QByteArray("?"); // QUESTION MARK
  QTest::newRow("0040") << QStringLiteral("\u0040") << QByteArray("@"); // COMMERCIAL AT
  QTest::newRow("0041") << QStringLiteral("\u0041") << QByteArray("A"); // LATIN CAPITAL LETTER A
  QTest::newRow("0042") << QStringLiteral("\u0042") << QByteArray("B"); // LATIN CAPITAL LETTER B
  QTest::newRow("0043") << QStringLiteral("\u0043") << QByteArray("C"); // LATIN CAPITAL LETTER C
  QTest::newRow("0044") << QStringLiteral("\u0044") << QByteArray("D"); // LATIN CAPITAL LETTER D
  QTest::newRow("0045") << QStringLiteral("\u0045") << QByteArray("E"); // LATIN CAPITAL LETTER E
  QTest::newRow("0046") << QStringLiteral("\u0046") << QByteArray("F"); // LATIN CAPITAL LETTER F
  QTest::newRow("0047") << QStringLiteral("\u0047") << QByteArray("G"); // LATIN CAPITAL LETTER G
  QTest::newRow("0048") << QStringLiteral("\u0048") << QByteArray("H"); // LATIN CAPITAL LETTER H
  QTest::newRow("0049") << QStringLiteral("\u0049") << QByteArray("I"); // LATIN CAPITAL LETTER I
  QTest::newRow("004a") << QStringLiteral("\u004a") << QByteArray("J"); // LATIN CAPITAL LETTER J
  QTest::newRow("004b") << QStringLiteral("\u004b") << QByteArray("K"); // LATIN CAPITAL LETTER K
  QTest::newRow("004c") << QStringLiteral("\u004c") << QByteArray("L"); // LATIN CAPITAL LETTER L
  QTest::newRow("004d") << QStringLiteral("\u004d") << QByteArray("M"); // LATIN CAPITAL LETTER M
  QTest::newRow("004e") << QStringLiteral("\u004e") << QByteArray("N"); // LATIN CAPITAL LETTER N
  QTest::newRow("004f") << QStringLiteral("\u004f") << QByteArray("O"); // LATIN CAPITAL LETTER O
  QTest::newRow("0050") << QStringLiteral("\u0050") << QByteArray("P"); // LATIN CAPITAL LETTER P
  QTest::newRow("0051") << QStringLiteral("\u0051") << QByteArray("Q"); // LATIN CAPITAL LETTER Q
  QTest::newRow("0052") << QStringLiteral("\u0052") << QByteArray("R"); // LATIN CAPITAL LETTER R
  QTest::newRow("0053") << QStringLiteral("\u0053") << QByteArray("S"); // LATIN CAPITAL LETTER S
  QTest::newRow("0054") << QStringLiteral("\u0054") << QByteArray("T"); // LATIN CAPITAL LETTER T
  QTest::newRow("0055") << QStringLiteral("\u0055") << QByteArray("U"); // LATIN CAPITAL LETTER U
  QTest::newRow("0056") << QStringLiteral("\u0056") << QByteArray("V"); // LATIN CAPITAL LETTER V
  QTest::newRow("0057") << QStringLiteral("\u0057") << QByteArray("W"); // LATIN CAPITAL LETTER W
  QTest::newRow("0058") << QStringLiteral("\u0058") << QByteArray("X"); // LATIN CAPITAL LETTER X
  QTest::newRow("0059") << QStringLiteral("\u0059") << QByteArray("Y"); // LATIN CAPITAL LETTER Y
  QTest::newRow("005a") << QStringLiteral("\u005a") << QByteArray("Z"); // LATIN CAPITAL LETTER Z
  QTest::newRow("005b") << QStringLiteral("\u005b") << QByteArray("["); // LEFT SQUARE BRACKET
  QTest::newRow("005c") << QStringLiteral("\u005c") << QByteArray("\\"); // REVERSE SOLIDUS
  QTest::newRow("005d") << QStringLiteral("\u005d") << QByteArray("]"); // RIGHT SQUARE BRACKET
  QTest::newRow("005e") << QStringLiteral("\u005e") << QByteArray("^"); // CIRCUMFLEX ACCENT
  QTest::newRow("005f") << QStringLiteral("\u005f") << QByteArray("_"); // LOW LINE
  QTest::newRow("0060") << QStringLiteral("\u0060") << QByteArray("`"); // GRAVE ACCENT
  QTest::newRow("0061") << QStringLiteral("\u0061") << QByteArray("a"); // LATIN SMALL LETTER A
  QTest::newRow("0062") << QStringLiteral("\u0062") << QByteArray("b"); // LATIN SMALL LETTER B
  QTest::newRow("0063") << QStringLiteral("\u0063") << QByteArray("c"); // LATIN SMALL LETTER C
  QTest::newRow("0064") << QStringLiteral("\u0064") << QByteArray("d"); // LATIN SMALL LETTER D
  QTest::newRow("0065") << QStringLiteral("\u0065") << QByteArray("e"); // LATIN SMALL LETTER E
  QTest::newRow("0066") << QStringLiteral("\u0066") << QByteArray("f"); // LATIN SMALL LETTER F
  QTest::newRow("0067") << QStringLiteral("\u0067") << QByteArray("g"); // LATIN SMALL LETTER G
  QTest::newRow("0068") << QStringLiteral("\u0068") << QByteArray("h"); // LATIN SMALL LETTER H
  QTest::newRow("0069") << QStringLiteral("\u0069") << QByteArray("i"); // LATIN SMALL LETTER I
  QTest::newRow("006a") << QStringLiteral("\u006a") << QByteArray("j"); // LATIN SMALL LETTER J
  QTest::newRow("006b") << QStringLiteral("\u006b") << QByteArray("k"); // LATIN SMALL LETTER K
  QTest::newRow("006c") << QStringLiteral("\u006c") << QByteArray("l"); // LATIN SMALL LETTER L
  QTest::newRow("006d") << QStringLiteral("\u006d") << QByteArray("m"); // LATIN SMALL LETTER M
  QTest::newRow("006e") << QStringLiteral("\u006e") << QByteArray("n"); // LATIN SMALL LETTER N
  QTest::newRow("006f") << QStringLiteral("\u006f") << QByteArray("o"); // LATIN SMALL LETTER O
  QTest::newRow("0070") << QStringLiteral("\u0070") << QByteArray("p"); // LATIN SMALL LETTER P
  QTest::newRow("0071") << QStringLiteral("\u0071") << QByteArray("q"); // LATIN SMALL LETTER Q
  QTest::newRow("0072") << QStringLiteral("\u0072") << QByteArray("r"); // LATIN SMALL LETTER R
  QTest::newRow("0073") << QStringLiteral("\u0073") << QByteArray("s"); // LATIN SMALL LETTER S
  QTest::newRow("0074") << QStringLiteral("\u0074") << QByteArray("t"); // LATIN SMALL LETTER T
  QTest::newRow("0075") << QStringLiteral("\u0075") << QByteArray("u"); // LATIN SMALL LETTER U
  QTest::newRow("0076") << QStringLiteral("\u0076") << QByteArray("v"); // LATIN SMALL LETTER V
  QTest::newRow("0077") << QStringLiteral("\u0077") << QByteArray("w"); // LATIN SMALL LETTER W
  QTest::newRow("0078") << QStringLiteral("\u0078") << QByteArray("x"); // LATIN SMALL LETTER X
  QTest::newRow("0079") << QStringLiteral("\u0079") << QByteArray("y"); // LATIN SMALL LETTER Y
  QTest::newRow("007a") << QStringLiteral("\u007a") << QByteArray("z"); // LATIN SMALL LETTER Z
  QTest::newRow("007b") << QStringLiteral("\u007b") << QByteArray("{"); // LEFT CURLY BRACKET
  QTest::newRow("007c") << QStringLiteral("\u007c") << QByteArray("|"); // VERTICAL LINE
  QTest::newRow("007d") << QStringLiteral("\u007d") << QByteArray("}"); // RIGHT CURLY BRACKET
  QTest::newRow("007e") << QStringLiteral("\u007e") << QByteArray("~"); // TILDE
  QTest::newRow("0088") << QStringLiteral("\u0088") << QByteArray("\x88"); // <control>
  QTest::newRow("0089") << QStringLiteral("\u0089") << QByteArray("\x89"); // <control>
  QTest::newRow("00a1") << QStringLiteral("\u00a1") << QByteArray("\xa1"); // INVERTED EXCLAMATION MARK
  QTest::newRow("00a3") << QStringLiteral("\u00a3") << QByteArray("\xa3"); // POUND SIGN
  QTest::newRow("00a5") << QStringLiteral("\u00a5") << QByteArray("\xa5"); // YEN SIGN
  QTest::newRow("00a7") << QStringLiteral("\u00a7") << QByteArray("\xa7"); // SECTION SIGN
  QTest::newRow("00a9") << QStringLiteral("\u00a9") << QByteArray("\xad"); // COPYRIGHT SIGN
  QTest::newRow("00ab") << QStringLiteral("\u00ab") << QByteArray("\xab"); // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
  QTest::newRow("00ae") << QStringLiteral("\u00ae") << QByteArray("\xaf"); // REGISTERED SIGN
  QTest::newRow("00b7") << QStringLiteral("\u00b7") << QByteArray("\xb7"); // MIDDLE DOT
  QTest::newRow("00bb") << QStringLiteral("\u00bb") << QByteArray("\xbb"); // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
  QTest::newRow("00bf") << QStringLiteral("\u00bf") << QByteArray("\xbf"); // INVERTED QUESTION MARK
  QTest::newRow("00c0") << QStringLiteral("\u00c0") << QByteArray("\xc1" "A"); // LATIN CAPITAL LETTER A WITH GRAVE
  QTest::newRow("00c1") << QStringLiteral("\u00c1") << QByteArray("\xc2" "A"); // LATIN CAPITAL LETTER A WITH ACUTE
  QTest::newRow("00c2") << QStringLiteral("\u00c2") << QByteArray("\xc3" "A"); // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
  QTest::newRow("00c3") << QStringLiteral("\u00c3") << QByteArray("\xc4" "A"); // LATIN CAPITAL LETTER A WITH TILDE
  QTest::newRow("00c4") << QStringLiteral("\u00c4") << QByteArray("\xc8" "A"); // LATIN CAPITAL LETTER A WITH DIAERESIS
  QTest::newRow("00c5") << QStringLiteral("\u00c5") << QByteArray("\xca" "A"); // LATIN CAPITAL LETTER A WITH RING ABOVE
  QTest::newRow("00c6") << QStringLiteral("\u00c6") << QByteArray("\xe1"); // LATIN CAPITAL LETTER AE
  QTest::newRow("00c7") << QStringLiteral("\u00c7") << QByteArray("\xd0" "C"); // LATIN CAPITAL LETTER C WITH CEDILLA
  QTest::newRow("00c8") << QStringLiteral("\u00c8") << QByteArray("\xc1" "E"); // LATIN CAPITAL LETTER E WITH GRAVE
  QTest::newRow("00c9") << QStringLiteral("\u00c9") << QByteArray("\xc2" "E"); // LATIN CAPITAL LETTER E WITH ACUTE
  QTest::newRow("00ca") << QStringLiteral("\u00ca") << QByteArray("\xc3" "E"); // LATIN CAPITAL LETTER E WITH CIRCUMFLEX
  QTest::newRow("00cb") << QStringLiteral("\u00cb") << QByteArray("\xc8" "E"); // LATIN CAPITAL LETTER E WITH DIAERESIS
  QTest::newRow("00cc") << QStringLiteral("\u00cc") << QByteArray("\xc1" "I"); // LATIN CAPITAL LETTER I WITH GRAVE
  QTest::newRow("00cd") << QStringLiteral("\u00cd") << QByteArray("\xc2" "I"); // LATIN CAPITAL LETTER I WITH ACUTE
  QTest::newRow("00ce") << QStringLiteral("\u00ce") << QByteArray("\xc3" "I"); // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
  QTest::newRow("00cf") << QStringLiteral("\u00cf") << QByteArray("\xc8" "I"); // LATIN CAPITAL LETTER I WITH DIAERESIS
  QTest::newRow("00d1") << QStringLiteral("\u00d1") << QByteArray("\xc4" "N"); // LATIN CAPITAL LETTER N WITH TILDE
  QTest::newRow("00d2") << QStringLiteral("\u00d2") << QByteArray("\xc1" "O"); // LATIN CAPITAL LETTER O WITH GRAVE
  QTest::newRow("00d3") << QStringLiteral("\u00d3") << QByteArray("\xc2" "O"); // LATIN CAPITAL LETTER O WITH ACUTE
  QTest::newRow("00d4") << QStringLiteral("\u00d4") << QByteArray("\xc3" "O"); // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
  QTest::newRow("00d5") << QStringLiteral("\u00d5") << QByteArray("\xc4" "O"); // LATIN CAPITAL LETTER O WITH TILDE
  QTest::newRow("00d6") << QStringLiteral("\u00d6") << QByteArray("\xc8" "O"); // LATIN CAPITAL LETTER O WITH DIAERESIS
  QTest::newRow("00d8") << QStringLiteral("\u00d8") << QByteArray("\xe9"); // LATIN CAPITAL LETTER O WITH STROKE
  QTest::newRow("00d9") << QStringLiteral("\u00d9") << QByteArray("\xc1" "U"); // LATIN CAPITAL LETTER U WITH GRAVE
  QTest::newRow("00da") << QStringLiteral("\u00da") << QByteArray("\xc2" "U"); // LATIN CAPITAL LETTER U WITH ACUTE
  QTest::newRow("00db") << QStringLiteral("\u00db") << QByteArray("\xc3" "U"); // LATIN CAPITAL LETTER U WITH CIRCUMFLEX
  QTest::newRow("00dc") << QStringLiteral("\u00dc") << QByteArray("\xc8" "U"); // LATIN CAPITAL LETTER U WITH DIAERESIS
  QTest::newRow("00dd") << QStringLiteral("\u00dd") << QByteArray("\xc2" "Y"); // LATIN CAPITAL LETTER Y WITH ACUTE
  QTest::newRow("00de") << QStringLiteral("\u00de") << QByteArray("\xec"); // LATIN CAPITAL LETTER THORN
  QTest::newRow("00df") << QStringLiteral("\u00df") << QByteArray("\xfb"); // LATIN SMALL LETTER SHARP S
  QTest::newRow("00e0") << QStringLiteral("\u00e0") << QByteArray("\xc1" "a"); // LATIN SMALL LETTER A WITH GRAVE
  QTest::newRow("00e1") << QStringLiteral("\u00e1") << QByteArray("\xc2" "a"); // LATIN SMALL LETTER A WITH ACUTE
  QTest::newRow("00e2") << QStringLiteral("\u00e2") << QByteArray("\xc3" "a"); // LATIN SMALL LETTER A WITH CIRCUMFLEX
  QTest::newRow("00e3") << QStringLiteral("\u00e3") << QByteArray("\xc4" "a"); // LATIN SMALL LETTER A WITH TILDE
  QTest::newRow("00e4") << QStringLiteral("\u00e4") << QByteArray("\xc8" "a"); // LATIN SMALL LETTER A WITH DIAERESIS
  QTest::newRow("00e5") << QStringLiteral("\u00e5") << QByteArray("\xca" "a"); // LATIN SMALL LETTER A WITH RING ABOVE
  QTest::newRow("00e6") << QStringLiteral("\u00e6") << QByteArray("\xf1"); // LATIN SMALL LETTER AE
  QTest::newRow("00e7") << QStringLiteral("\u00e7") << QByteArray("\xd0" "c"); // LATIN SMALL LETTER C WITH CEDILLA
  QTest::newRow("00e8") << QStringLiteral("\u00e8") << QByteArray("\xc1" "e"); // LATIN SMALL LETTER E WITH GRAVE
  QTest::newRow("00e9") << QStringLiteral("\u00e9") << QByteArray("\xc2" "e"); // LATIN SMALL LETTER E WITH ACUTE
  QTest::newRow("00ea") << QStringLiteral("\u00ea") << QByteArray("\xc3" "e"); // LATIN SMALL LETTER E WITH CIRCUMFLEX
  QTest::newRow("00eb") << QStringLiteral("\u00eb") << QByteArray("\xc8" "e"); // LATIN SMALL LETTER E WITH DIAERESIS
  QTest::newRow("00ec") << QStringLiteral("\u00ec") << QByteArray("\xc1" "i"); // LATIN SMALL LETTER I WITH GRAVE
  QTest::newRow("00ed") << QStringLiteral("\u00ed") << QByteArray("\xc2" "i"); // LATIN SMALL LETTER I WITH ACUTE
  QTest::newRow("00ee") << QStringLiteral("\u00ee") << QByteArray("\xc3" "i"); // LATIN SMALL LETTER I WITH CIRCUMFLEX
  QTest::newRow("00ef") << QStringLiteral("\u00ef") << QByteArray("\xc8" "i"); // LATIN SMALL LETTER I WITH DIAERESIS
  QTest::newRow("00f0") << QStringLiteral("\u00f0") << QByteArray("\xf3"); // LATIN SMALL LETTER ETH
  QTest::newRow("00f1") << QStringLiteral("\u00f1") << QByteArray("\xc4" "n"); // LATIN SMALL LETTER N WITH TILDE
  QTest::newRow("00f2") << QStringLiteral("\u00f2") << QByteArray("\xc1" "o"); // LATIN SMALL LETTER O WITH GRAVE
  QTest::newRow("00f3") << QStringLiteral("\u00f3") << QByteArray("\xc2" "o"); // LATIN SMALL LETTER O WITH ACUTE
  QTest::newRow("00f4") << QStringLiteral("\u00f4") << QByteArray("\xc3" "o"); // LATIN SMALL LETTER O WITH CIRCUMFLEX
  QTest::newRow("00f5") << QStringLiteral("\u00f5") << QByteArray("\xc4" "o"); // LATIN SMALL LETTER O WITH TILDE
  QTest::newRow("00f6") << QStringLiteral("\u00f6") << QByteArray("\xc8" "o"); // LATIN SMALL LETTER O WITH DIAERESIS
  QTest::newRow("00f8") << QStringLiteral("\u00f8") << QByteArray("\xf9"); // LATIN SMALL LETTER O WITH STROKE
  QTest::newRow("00f9") << QStringLiteral("\u00f9") << QByteArray("\xc1" "u"); // LATIN SMALL LETTER U WITH GRAVE
  QTest::newRow("00fa") << QStringLiteral("\u00fa") << QByteArray("\xc2" "u"); // LATIN SMALL LETTER U WITH ACUTE
  QTest::newRow("00fb") << QStringLiteral("\u00fb") << QByteArray("\xc3" "u"); // LATIN SMALL LETTER U WITH CIRCUMFLEX
  QTest::newRow("00fc") << QStringLiteral("\u00fc") << QByteArray("\xc8" "u"); // LATIN SMALL LETTER U WITH DIAERESIS
  QTest::newRow("00fd") << QStringLiteral("\u00fd") << QByteArray("\xc2" "y"); // LATIN SMALL LETTER Y WITH ACUTE
  QTest::newRow("00fe") << QStringLiteral("\u00fe") << QByteArray("\xfc"); // LATIN SMALL LETTER THORN
  QTest::newRow("00ff") << QStringLiteral("\u00ff") << QByteArray("\xc8" "y"); // LATIN SMALL LETTER Y WITH DIAERESIS
  QTest::newRow("0100") << QStringLiteral("\u0100") << QByteArray("\xc5" "A"); // LATIN CAPITAL LETTER A WITH MACRON
  QTest::newRow("0101") << QStringLiteral("\u0101") << QByteArray("\xc5" "a"); // LATIN SMALL LETTER A WITH MACRON
  QTest::newRow("0102") << QStringLiteral("\u0102") << QByteArray("\xc6" "A"); // LATIN CAPITAL LETTER A WITH BREVE
  QTest::newRow("0103") << QStringLiteral("\u0103") << QByteArray("\xc6" "a"); // LATIN SMALL LETTER A WITH BREVE
  QTest::newRow("0104") << QStringLiteral("\u0104") << QByteArray("\xd3" "A"); // LATIN CAPITAL LETTER A WITH OGONEK
  QTest::newRow("0105") << QStringLiteral("\u0105") << QByteArray("\xd3" "a"); // LATIN SMALL LETTER A WITH OGONEK
  QTest::newRow("0106") << QStringLiteral("\u0106") << QByteArray("\xc2" "C"); // LATIN CAPITAL LETTER C WITH ACUTE
  QTest::newRow("0107") << QStringLiteral("\u0107") << QByteArray("\xc2" "c"); // LATIN SMALL LETTER C WITH ACUTE
  QTest::newRow("0108") << QStringLiteral("\u0108") << QByteArray("\xc3" "C"); // LATIN CAPITAL LETTER C WITH CIRCUMFLEX
  QTest::newRow("0109") << QStringLiteral("\u0109") << QByteArray("\xc3" "c"); // LATIN SMALL LETTER C WITH CIRCUMFLEX
  QTest::newRow("010a") << QStringLiteral("\u010a") << QByteArray("\xc7" "C"); // LATIN CAPITAL LETTER C WITH DOT ABOVE
  QTest::newRow("010b") << QStringLiteral("\u010b") << QByteArray("\xc7" "c"); // LATIN SMALL LETTER C WITH DOT ABOVE
  QTest::newRow("010c") << QStringLiteral("\u010c") << QByteArray("\xcf" "C"); // LATIN CAPITAL LETTER C WITH CARON
  QTest::newRow("010d") << QStringLiteral("\u010d") << QByteArray("\xcf" "c"); // LATIN SMALL LETTER C WITH CARON
  QTest::newRow("010e") << QStringLiteral("\u010e") << QByteArray("\xcf" "D"); // LATIN CAPITAL LETTER D WITH CARON
  QTest::newRow("010f") << QStringLiteral("\u010f") << QByteArray("\xcf" "d"); // LATIN SMALL LETTER D WITH CARON
  QTest::newRow("0110") << QStringLiteral("\u0110") << QByteArray("\xe2"); // LATIN CAPITAL LETTER D WITH STROKE
  QTest::newRow("0111") << QStringLiteral("\u0111") << QByteArray("\xf2"); // LATIN SMALL LETTER D WITH STROKE
  QTest::newRow("0112") << QStringLiteral("\u0112") << QByteArray("\xc5" "E"); // LATIN CAPITAL LETTER E WITH MACRON
  QTest::newRow("0113") << QStringLiteral("\u0113") << QByteArray("\xc5" "e"); // LATIN SMALL LETTER E WITH MACRON
  QTest::newRow("0114") << QStringLiteral("\u0114") << QByteArray("\xc6" "E"); // LATIN CAPITAL LETTER E WITH BREVE
  QTest::newRow("0115") << QStringLiteral("\u0115") << QByteArray("\xc6" "e"); // LATIN SMALL LETTER E WITH BREVE
  QTest::newRow("0116") << QStringLiteral("\u0116") << QByteArray("\xc7" "E"); // LATIN CAPITAL LETTER E WITH DOT ABOVE
  QTest::newRow("0117") << QStringLiteral("\u0117") << QByteArray("\xc7" "e"); // LATIN SMALL LETTER E WITH DOT ABOVE
  QTest::newRow("0118") << QStringLiteral("\u0118") << QByteArray("\xd3" "E"); // LATIN CAPITAL LETTER E WITH OGONEK
  QTest::newRow("0119") << QStringLiteral("\u0119") << QByteArray("\xd3" "e"); // LATIN SMALL LETTER E WITH OGONEK
  QTest::newRow("011a") << QStringLiteral("\u011a") << QByteArray("\xcf" "E"); // LATIN CAPITAL LETTER E WITH CARON
  QTest::newRow("011b") << QStringLiteral("\u011b") << QByteArray("\xcf" "e"); // LATIN SMALL LETTER E WITH CARON
  QTest::newRow("011c") << QStringLiteral("\u011c") << QByteArray("\xc3" "G"); // LATIN CAPITAL LETTER G WITH CIRCUMFLEX
  QTest::newRow("011d") << QStringLiteral("\u011d") << QByteArray("\xc3" "g"); // LATIN SMALL LETTER G WITH CIRCUMFLEX
  QTest::newRow("011e") << QStringLiteral("\u011e") << QByteArray("\xc6" "G"); // LATIN CAPITAL LETTER G WITH BREVE
  QTest::newRow("011f") << QStringLiteral("\u011f") << QByteArray("\xc6" "g"); // LATIN SMALL LETTER G WITH BREVE
  QTest::newRow("0120") << QStringLiteral("\u0120") << QByteArray("\xc7" "G"); // LATIN CAPITAL LETTER G WITH DOT ABOVE
  QTest::newRow("0121") << QStringLiteral("\u0121") << QByteArray("\xc7" "g"); // LATIN SMALL LETTER G WITH DOT ABOVE
  QTest::newRow("0122") << QStringLiteral("\u0122") << QByteArray("\xd0" "G"); // LATIN CAPITAL LETTER G WITH CEDILLA
  QTest::newRow("0123") << QStringLiteral("\u0123") << QByteArray("\xd0" "g"); // LATIN SMALL LETTER G WITH CEDILLA
  QTest::newRow("0124") << QStringLiteral("\u0124") << QByteArray("\xc3" "H"); // LATIN CAPITAL LETTER H WITH CIRCUMFLEX
  QTest::newRow("0125") << QStringLiteral("\u0125") << QByteArray("\xc3" "h"); // LATIN SMALL LETTER H WITH CIRCUMFLEX
  QTest::newRow("0128") << QStringLiteral("\u0128") << QByteArray("\xc4" "I"); // LATIN CAPITAL LETTER I WITH TILDE
  QTest::newRow("0129") << QStringLiteral("\u0129") << QByteArray("\xc4" "i"); // LATIN SMALL LETTER I WITH TILDE
  QTest::newRow("012a") << QStringLiteral("\u012a") << QByteArray("\xc5" "I"); // LATIN CAPITAL LETTER I WITH MACRON
  QTest::newRow("012b") << QStringLiteral("\u012b") << QByteArray("\xc5" "i"); // LATIN SMALL LETTER I WITH MACRON
  QTest::newRow("012c") << QStringLiteral("\u012c") << QByteArray("\xc6" "I"); // LATIN CAPITAL LETTER I WITH BREVE
  QTest::newRow("012d") << QStringLiteral("\u012d") << QByteArray("\xc6" "i"); // LATIN SMALL LETTER I WITH BREVE
  QTest::newRow("012e") << QStringLiteral("\u012e") << QByteArray("\xd3" "I"); // LATIN CAPITAL LETTER I WITH OGONEK
  QTest::newRow("012f") << QStringLiteral("\u012f") << QByteArray("\xd3" "i"); // LATIN SMALL LETTER I WITH OGONEK
  QTest::newRow("0130") << QStringLiteral("\u0130") << QByteArray("\xc7" "I"); // LATIN CAPITAL LETTER I WITH DOT ABOVE
  QTest::newRow("0131") << QStringLiteral("\u0131") << QByteArray("\xf5"); // LATIN SMALL LETTER DOTLESS I
  QTest::newRow("0132") << QStringLiteral("\u0132") << QByteArray("\xe6"); // LATIN CAPITAL LIGATURE IJ
  QTest::newRow("0133") << QStringLiteral("\u0133") << QByteArray("\xf6"); // LATIN SMALL LIGATURE IJ
  QTest::newRow("0134") << QStringLiteral("\u0134") << QByteArray("\xc3" "J"); // LATIN CAPITAL LETTER J WITH CIRCUMFLEX
  QTest::newRow("0135") << QStringLiteral("\u0135") << QByteArray("\xc3" "j"); // LATIN SMALL LETTER J WITH CIRCUMFLEX
  QTest::newRow("0136") << QStringLiteral("\u0136") << QByteArray("\xd0" "K"); // LATIN CAPITAL LETTER K WITH CEDILLA
  QTest::newRow("0137") << QStringLiteral("\u0137") << QByteArray("\xd0" "k"); // LATIN SMALL LETTER K WITH CEDILLA
  QTest::newRow("0139") << QStringLiteral("\u0139") << QByteArray("\xc2" "L"); // LATIN CAPITAL LETTER L WITH ACUTE
  QTest::newRow("013a") << QStringLiteral("\u013a") << QByteArray("\xc2" "l"); // LATIN SMALL LETTER L WITH ACUTE
  QTest::newRow("013b") << QStringLiteral("\u013b") << QByteArray("\xd0" "L"); // LATIN CAPITAL LETTER L WITH CEDILLA
  QTest::newRow("013c") << QStringLiteral("\u013c") << QByteArray("\xd0" "l"); // LATIN SMALL LETTER L WITH CEDILLA
  QTest::newRow("013d") << QStringLiteral("\u013d") << QByteArray("\xcf" "L"); // LATIN CAPITAL LETTER L WITH CARON
  QTest::newRow("013e") << QStringLiteral("\u013e") << QByteArray("\xcf" "l"); // LATIN SMALL LETTER L WITH CARON
  QTest::newRow("0141") << QStringLiteral("\u0141") << QByteArray("\xe8"); // LATIN CAPITAL LETTER L WITH STROKE
  QTest::newRow("0142") << QStringLiteral("\u0142") << QByteArray("\xf8"); // LATIN SMALL LETTER L WITH STROKE
  QTest::newRow("0143") << QStringLiteral("\u0143") << QByteArray("\xc2" "N"); // LATIN CAPITAL LETTER N WITH ACUTE
  QTest::newRow("0144") << QStringLiteral("\u0144") << QByteArray("\xc2" "n"); // LATIN SMALL LETTER N WITH ACUTE
  QTest::newRow("0145") << QStringLiteral("\u0145") << QByteArray("\xd0" "N"); // LATIN CAPITAL LETTER N WITH CEDILLA
  QTest::newRow("0146") << QStringLiteral("\u0146") << QByteArray("\xd0" "n"); // LATIN SMALL LETTER N WITH CEDILLA
  QTest::newRow("0147") << QStringLiteral("\u0147") << QByteArray("\xcf" "N"); // LATIN CAPITAL LETTER N WITH CARON
  QTest::newRow("0148") << QStringLiteral("\u0148") << QByteArray("\xcf" "n"); // LATIN SMALL LETTER N WITH CARON
  QTest::newRow("014c") << QStringLiteral("\u014c") << QByteArray("\xc5" "O"); // LATIN CAPITAL LETTER O WITH MACRON
  QTest::newRow("014d") << QStringLiteral("\u014d") << QByteArray("\xc5" "o"); // LATIN SMALL LETTER O WITH MACRON
  QTest::newRow("014e") << QStringLiteral("\u014e") << QByteArray("\xc6" "O"); // LATIN CAPITAL LETTER O WITH BREVE
  QTest::newRow("014f") << QStringLiteral("\u014f") << QByteArray("\xc6" "o"); // LATIN SMALL LETTER O WITH BREVE
  QTest::newRow("0150") << QStringLiteral("\u0150") << QByteArray("\xcd" "O"); // LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
  QTest::newRow("0151") << QStringLiteral("\u0151") << QByteArray("\xcd" "o"); // LATIN SMALL LETTER O WITH DOUBLE ACUTE
  QTest::newRow("0152") << QStringLiteral("\u0152") << QByteArray("\xea"); // LATIN CAPITAL LIGATURE OE
  QTest::newRow("0153") << QStringLiteral("\u0153") << QByteArray("\xfa"); // LATIN SMALL LIGATURE OE
  QTest::newRow("0154") << QStringLiteral("\u0154") << QByteArray("\xc2" "R"); // LATIN CAPITAL LETTER R WITH ACUTE
  QTest::newRow("0155") << QStringLiteral("\u0155") << QByteArray("\xc2" "r"); // LATIN SMALL LETTER R WITH ACUTE
  QTest::newRow("0156") << QStringLiteral("\u0156") << QByteArray("\xd0" "R"); // LATIN CAPITAL LETTER R WITH CEDILLA
  QTest::newRow("0157") << QStringLiteral("\u0157") << QByteArray("\xd0" "r"); // LATIN SMALL LETTER R WITH CEDILLA
  QTest::newRow("0158") << QStringLiteral("\u0158") << QByteArray("\xcf" "R"); // LATIN CAPITAL LETTER R WITH CARON
  QTest::newRow("0159") << QStringLiteral("\u0159") << QByteArray("\xcf" "r"); // LATIN SMALL LETTER R WITH CARON
  QTest::newRow("015a") << QStringLiteral("\u015a") << QByteArray("\xc2" "S"); // LATIN CAPITAL LETTER S WITH ACUTE
  QTest::newRow("015b") << QStringLiteral("\u015b") << QByteArray("\xc2" "s"); // LATIN SMALL LETTER S WITH ACUTE
  QTest::newRow("015c") << QStringLiteral("\u015c") << QByteArray("\xc3" "S"); // LATIN CAPITAL LETTER S WITH CIRCUMFLEX
  QTest::newRow("015d") << QStringLiteral("\u015d") << QByteArray("\xc3" "s"); // LATIN SMALL LETTER S WITH CIRCUMFLEX
  QTest::newRow("015e") << QStringLiteral("\u015e") << QByteArray("\xd0" "S"); // LATIN CAPITAL LETTER S WITH CEDILLA
  QTest::newRow("015f") << QStringLiteral("\u015f") << QByteArray("\xd0" "s"); // LATIN SMALL LETTER S WITH CEDILLA
  QTest::newRow("0160") << QStringLiteral("\u0160") << QByteArray("\xcf" "S"); // LATIN CAPITAL LETTER S WITH CARON
  QTest::newRow("0161") << QStringLiteral("\u0161") << QByteArray("\xcf" "s"); // LATIN SMALL LETTER S WITH CARON
  QTest::newRow("0162") << QStringLiteral("\u0162") << QByteArray("\xd0" "T"); // LATIN CAPITAL LETTER T WITH CEDILLA
  QTest::newRow("0163") << QStringLiteral("\u0163") << QByteArray("\xd0" "t"); // LATIN SMALL LETTER T WITH CEDILLA
  QTest::newRow("0164") << QStringLiteral("\u0164") << QByteArray("\xcf" "T"); // LATIN CAPITAL LETTER T WITH CARON
  QTest::newRow("0165") << QStringLiteral("\u0165") << QByteArray("\xcf" "t"); // LATIN SMALL LETTER T WITH CARON
  QTest::newRow("0168") << QStringLiteral("\u0168") << QByteArray("\xc4" "U"); // LATIN CAPITAL LETTER U WITH TILDE
  QTest::newRow("0169") << QStringLiteral("\u0169") << QByteArray("\xc4" "u"); // LATIN SMALL LETTER U WITH TILDE
  QTest::newRow("016a") << QStringLiteral("\u016a") << QByteArray("\xc5" "U"); // LATIN CAPITAL LETTER U WITH MACRON
  QTest::newRow("016b") << QStringLiteral("\u016b") << QByteArray("\xc5" "u"); // LATIN SMALL LETTER U WITH MACRON
  QTest::newRow("016c") << QStringLiteral("\u016c") << QByteArray("\xc6" "U"); // LATIN CAPITAL LETTER U WITH BREVE
  QTest::newRow("016d") << QStringLiteral("\u016d") << QByteArray("\xc6" "u"); // LATIN SMALL LETTER U WITH BREVE
  QTest::newRow("016e") << QStringLiteral("\u016e") << QByteArray("\xca" "U"); // LATIN CAPITAL LETTER U WITH RING ABOVE
  QTest::newRow("016f") << QStringLiteral("\u016f") << QByteArray("\xca" "u"); // LATIN SMALL LETTER U WITH RING ABOVE
  QTest::newRow("0170") << QStringLiteral("\u0170") << QByteArray("\xcd" "U"); // LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
  QTest::newRow("0171") << QStringLiteral("\u0171") << QByteArray("\xcd" "u"); // LATIN SMALL LETTER U WITH DOUBLE ACUTE
  QTest::newRow("0172") << QStringLiteral("\u0172") << QByteArray("\xd3" "U"); // LATIN CAPITAL LETTER U WITH OGONEK
  QTest::newRow("0173") << QStringLiteral("\u0173") << QByteArray("\xd3" "u"); // LATIN SMALL LETTER U WITH OGONEK
  QTest::newRow("0174") << QStringLiteral("\u0174") << QByteArray("\xc3" "W"); // LATIN CAPITAL LETTER W WITH CIRCUMFLEX
  QTest::newRow("0175") << QStringLiteral("\u0175") << QByteArray("\xc3" "w"); // LATIN SMALL LETTER W WITH CIRCUMFLEX
  QTest::newRow("0176") << QStringLiteral("\u0176") << QByteArray("\xc3" "Y"); // LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
  QTest::newRow("0177") << QStringLiteral("\u0177") << QByteArray("\xc3" "y"); // LATIN SMALL LETTER Y WITH CIRCUMFLEX
  QTest::newRow("0178") << QStringLiteral("\u0178") << QByteArray("\xc8" "Y"); // LATIN CAPITAL LETTER Y WITH DIAERESIS
  QTest::newRow("0179") << QStringLiteral("\u0179") << QByteArray("\xc2" "Z"); // LATIN CAPITAL LETTER Z WITH ACUTE
  QTest::newRow("017a") << QStringLiteral("\u017a") << QByteArray("\xc2" "z"); // LATIN SMALL LETTER Z WITH ACUTE
  QTest::newRow("017b") << QStringLiteral("\u017b") << QByteArray("\xc7" "Z"); // LATIN CAPITAL LETTER Z WITH DOT ABOVE
  QTest::newRow("017c") << QStringLiteral("\u017c") << QByteArray("\xc7" "z"); // LATIN SMALL LETTER Z WITH DOT ABOVE
  QTest::newRow("017d") << QStringLiteral("\u017d") << QByteArray("\xcf" "Z"); // LATIN CAPITAL LETTER Z WITH CARON
  QTest::newRow("017e") << QStringLiteral("\u017e") << QByteArray("\xcf" "z"); // LATIN SMALL LETTER Z WITH CARON
  QTest::newRow("01a0") << QStringLiteral("\u01a0") << QByteArray("\xce" "O"); // LATIN CAPITAL LETTER O WITH HORN
  QTest::newRow("01a1") << QStringLiteral("\u01a1") << QByteArray("\xce" "o"); // LATIN SMALL LETTER O WITH HORN
  QTest::newRow("01af") << QStringLiteral("\u01af") << QByteArray("\xce" "U"); // LATIN CAPITAL LETTER U WITH HORN
  QTest::newRow("01b0") << QStringLiteral("\u01b0") << QByteArray("\xce" "u"); // LATIN SMALL LETTER U WITH HORN
  QTest::newRow("01cd") << QStringLiteral("\u01cd") << QByteArray("\xcf" "A"); // LATIN CAPITAL LETTER A WITH CARON
  QTest::newRow("01ce") << QStringLiteral("\u01ce") << QByteArray("\xcf" "a"); // LATIN SMALL LETTER A WITH CARON
  QTest::newRow("01cf") << QStringLiteral("\u01cf") << QByteArray("\xcf" "I"); // LATIN CAPITAL LETTER I WITH CARON
  QTest::newRow("01d0") << QStringLiteral("\u01d0") << QByteArray("\xcf" "i"); // LATIN SMALL LETTER I WITH CARON
  QTest::newRow("01d1") << QStringLiteral("\u01d1") << QByteArray("\xcf" "O"); // LATIN CAPITAL LETTER O WITH CARON
  QTest::newRow("01d2") << QStringLiteral("\u01d2") << QByteArray("\xcf" "o"); // LATIN SMALL LETTER O WITH CARON
  QTest::newRow("01d3") << QStringLiteral("\u01d3") << QByteArray("\xcf" "U"); // LATIN CAPITAL LETTER U WITH CARON
  QTest::newRow("01d4") << QStringLiteral("\u01d4") << QByteArray("\xcf" "u"); // LATIN SMALL LETTER U WITH CARON
  QTest::newRow("01d5") << QStringLiteral("\u01d5") << QByteArray("\xc5\xc8" "U"); // LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON
  QTest::newRow("01d6") << QStringLiteral("\u01d6") << QByteArray("\xc5\xc8" "u"); // LATIN SMALL LETTER U WITH DIAERESIS AND MACRON
  QTest::newRow("01d7") << QStringLiteral("\u01d7") << QByteArray("\xc2\xc8" "U"); // LATIN CAPITAL LETTER U WITH DIAERESIS AND ACUTE
  QTest::newRow("01d8") << QStringLiteral("\u01d8") << QByteArray("\xc2\xc8" "u"); // LATIN SMALL LETTER U WITH DIAERESIS AND ACUTE
  QTest::newRow("01d9") << QStringLiteral("\u01d9") << QByteArray("\xcf\xc8" "U"); // LATIN CAPITAL LETTER U WITH DIAERESIS AND CARON
  QTest::newRow("01da") << QStringLiteral("\u01da") << QByteArray("\xcf\xc8" "u"); // LATIN SMALL LETTER U WITH DIAERESIS AND CARON
  QTest::newRow("01db") << QStringLiteral("\u01db") << QByteArray("\xc1\xc8" "U"); // LATIN CAPITAL LETTER U WITH DIAERESIS AND GRAVE
  QTest::newRow("01dc") << QStringLiteral("\u01dc") << QByteArray("\xc1\xc8" "u"); // LATIN SMALL LETTER U WITH DIAERESIS AND GRAVE
  QTest::newRow("01de") << QStringLiteral("\u01de") << QByteArray("\xc5\xc8" "A"); // LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON
  QTest::newRow("01df") << QStringLiteral("\u01df") << QByteArray("\xc5\xc8" "a"); // LATIN SMALL LETTER A WITH DIAERESIS AND MACRON
  QTest::newRow("01e0") << QStringLiteral("\u01e0") << QByteArray("\xc5\xc7" "A"); // LATIN CAPITAL LETTER A WITH DOT ABOVE AND MACRON
  QTest::newRow("01e1") << QStringLiteral("\u01e1") << QByteArray("\xc5\xc7" "a"); // LATIN SMALL LETTER A WITH DOT ABOVE AND MACRON
  QTest::newRow("01e2") << QStringLiteral("\u01e2") << QByteArray("\xc5\xe1"); // LATIN CAPITAL LETTER AE WITH MACRON
  QTest::newRow("01e3") << QStringLiteral("\u01e3") << QByteArray("\xc5\xf1"); // LATIN SMALL LETTER AE WITH MACRON
  QTest::newRow("01e6") << QStringLiteral("\u01e6") << QByteArray("\xcf" "G"); // LATIN CAPITAL LETTER G WITH CARON
  QTest::newRow("01e7") << QStringLiteral("\u01e7") << QByteArray("\xcf" "g"); // LATIN SMALL LETTER G WITH CARON
  QTest::newRow("01e8") << QStringLiteral("\u01e8") << QByteArray("\xcf" "K"); // LATIN CAPITAL LETTER K WITH CARON
  QTest::newRow("01e9") << QStringLiteral("\u01e9") << QByteArray("\xcf" "k"); // LATIN SMALL LETTER K WITH CARON
  QTest::newRow("01ea") << QStringLiteral("\u01ea") << QByteArray("\xd3" "O"); // LATIN CAPITAL LETTER O WITH OGONEK
  QTest::newRow("01eb") << QStringLiteral("\u01eb") << QByteArray("\xd3" "o"); // LATIN SMALL LETTER O WITH OGONEK
  QTest::newRow("01ec") << QStringLiteral("\u01ec") << QByteArray("\xc5\xd3" "O"); // LATIN CAPITAL LETTER O WITH OGONEK AND MACRON
  QTest::newRow("01ed") << QStringLiteral("\u01ed") << QByteArray("\xc5\xd3" "o"); // LATIN SMALL LETTER O WITH OGONEK AND MACRON
  QTest::newRow("01f0") << QStringLiteral("\u01f0") << QByteArray("\xcf" "j"); // LATIN SMALL LETTER J WITH CARON
  QTest::newRow("01f4") << QStringLiteral("\u01f4") << QByteArray("\xc2" "G"); // LATIN CAPITAL LETTER G WITH ACUTE
  QTest::newRow("01f5") << QStringLiteral("\u01f5") << QByteArray("\xc2" "g"); // LATIN SMALL LETTER G WITH ACUTE
  QTest::newRow("01f8") << QStringLiteral("\u01f8") << QByteArray("\xc1" "N"); // LATIN CAPITAL LETTER N WITH GRAVE
  QTest::newRow("01f9") << QStringLiteral("\u01f9") << QByteArray("\xc1" "n"); // LATIN SMALL LETTER N WITH GRAVE
  QTest::newRow("01fa") << QStringLiteral("\u01fa") << QByteArray("\xc2\xca" "A"); // LATIN CAPITAL LETTER A WITH RING ABOVE AND ACUTE
  QTest::newRow("01fb") << QStringLiteral("\u01fb") << QByteArray("\xc2\xca" "a"); // LATIN SMALL LETTER A WITH RING ABOVE AND ACUTE
  QTest::newRow("01fc") << QStringLiteral("\u01fc") << QByteArray("\xc2\xe1"); // LATIN CAPITAL LETTER AE WITH ACUTE
  QTest::newRow("01fd") << QStringLiteral("\u01fd") << QByteArray("\xc2\xf1"); // LATIN SMALL LETTER AE WITH ACUTE
  QTest::newRow("01fe") << QStringLiteral("\u01fe") << QByteArray("\xc2\xe9"); // LATIN CAPITAL LETTER O WITH STROKE AND ACUTE
  QTest::newRow("01ff") << QStringLiteral("\u01ff") << QByteArray("\xc2\xf9"); // LATIN SMALL LETTER O WITH STROKE AND ACUTE
  QTest::newRow("0218") << QStringLiteral("\u0218") << QByteArray("\xd2" "S"); // LATIN CAPITAL LETTER S WITH COMMA BELOW
  QTest::newRow("0219") << QStringLiteral("\u0219") << QByteArray("\xd2" "s"); // LATIN SMALL LETTER S WITH COMMA BELOW
  QTest::newRow("021a") << QStringLiteral("\u021a") << QByteArray("\xd2" "T"); // LATIN CAPITAL LETTER T WITH COMMA BELOW
  QTest::newRow("021b") << QStringLiteral("\u021b") << QByteArray("\xd2" "t"); // LATIN SMALL LETTER T WITH COMMA BELOW
  QTest::newRow("021e") << QStringLiteral("\u021e") << QByteArray("\xcf" "H"); // LATIN CAPITAL LETTER H WITH CARON
  QTest::newRow("021f") << QStringLiteral("\u021f") << QByteArray("\xcf" "h"); // LATIN SMALL LETTER H WITH CARON
  QTest::newRow("0226") << QStringLiteral("\u0226") << QByteArray("\xc7" "A"); // LATIN CAPITAL LETTER A WITH DOT ABOVE
  QTest::newRow("0227") << QStringLiteral("\u0227") << QByteArray("\xc7" "a"); // LATIN SMALL LETTER A WITH DOT ABOVE
  QTest::newRow("0228") << QStringLiteral("\u0228") << QByteArray("\xd0" "E"); // LATIN CAPITAL LETTER E WITH CEDILLA
  QTest::newRow("0229") << QStringLiteral("\u0229") << QByteArray("\xd0" "e"); // LATIN SMALL LETTER E WITH CEDILLA
  QTest::newRow("022a") << QStringLiteral("\u022a") << QByteArray("\xc5\xc8" "O"); // LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON
  QTest::newRow("022b") << QStringLiteral("\u022b") << QByteArray("\xc5\xc8" "o"); // LATIN SMALL LETTER O WITH DIAERESIS AND MACRON
  QTest::newRow("022c") << QStringLiteral("\u022c") << QByteArray("\xc5\xc4" "O"); // LATIN CAPITAL LETTER O WITH TILDE AND MACRON
  QTest::newRow("022d") << QStringLiteral("\u022d") << QByteArray("\xc5\xc4" "o"); // LATIN SMALL LETTER O WITH TILDE AND MACRON
  QTest::newRow("022e") << QStringLiteral("\u022e") << QByteArray("\xc7" "O"); // LATIN CAPITAL LETTER O WITH DOT ABOVE
  QTest::newRow("022f") << QStringLiteral("\u022f") << QByteArray("\xc7" "o"); // LATIN SMALL LETTER O WITH DOT ABOVE
  QTest::newRow("0230") << QStringLiteral("\u0230") << QByteArray("\xc5\xc7" "O"); // LATIN CAPITAL LETTER O WITH DOT ABOVE AND MACRON
  QTest::newRow("0231") << QStringLiteral("\u0231") << QByteArray("\xc5\xc7" "o"); // LATIN SMALL LETTER O WITH DOT ABOVE AND MACRON
  QTest::newRow("0232") << QStringLiteral("\u0232") << QByteArray("\xc5" "Y"); // LATIN CAPITAL LETTER Y WITH MACRON
  QTest::newRow("0233") << QStringLiteral("\u0233") << QByteArray("\xc5" "y"); // LATIN SMALL LETTER Y WITH MACRON
  QTest::newRow("02b9") << QStringLiteral("\u02b9") << QByteArray("\xbd"); // MODIFIER LETTER PRIME
  QTest::newRow("02ba") << QStringLiteral("\u02ba") << QByteArray("\xbe"); // MODIFIER LETTER DOUBLE PRIME
//  QTest::newRow("02bb") << QStringLiteral("\u02bb") << QByteArray("\xb0"); // MODIFIER LETTER TURNED COMMA
//  QTest::newRow("02bc") << QStringLiteral("\u02bc") << QByteArray("\xb1"); // MODIFIER LETTER APOSTROPHE
//  QTest::newRow("0374") << QStringLiteral("\u0374") << QByteArray("\xbd"); // GREEK NUMERAL SIGN
//  QTest::newRow("037e") << QStringLiteral("\u037e") << QByteArray(";"); // GREEK QUESTION MARK
//  QTest::newRow("0387") << QStringLiteral("\u0387") << QByteArray("\xb7"); // GREEK ANO TELEIA
  QTest::newRow("1e00") << QStringLiteral("\u1e00") << QByteArray("\xd4" "A"); // LATIN CAPITAL LETTER A WITH RING BELOW
  QTest::newRow("1e01") << QStringLiteral("\u1e01") << QByteArray("\xd4" "a"); // LATIN SMALL LETTER A WITH RING BELOW
  QTest::newRow("1e02") << QStringLiteral("\u1e02") << QByteArray("\xc7" "B"); // LATIN CAPITAL LETTER B WITH DOT ABOVE
  QTest::newRow("1e03") << QStringLiteral("\u1e03") << QByteArray("\xc7" "b"); // LATIN SMALL LETTER B WITH DOT ABOVE
  QTest::newRow("1e04") << QStringLiteral("\u1e04") << QByteArray("\xd6" "B"); // LATIN CAPITAL LETTER B WITH DOT BELOW
  QTest::newRow("1e05") << QStringLiteral("\u1e05") << QByteArray("\xd6" "b"); // LATIN SMALL LETTER B WITH DOT BELOW
  QTest::newRow("1e08") << QStringLiteral("\u1e08") << QByteArray("\xc2\xd0" "C"); // LATIN CAPITAL LETTER C WITH CEDILLA AND ACUTE
  QTest::newRow("1e09") << QStringLiteral("\u1e09") << QByteArray("\xc2\xd0" "c"); // LATIN SMALL LETTER C WITH CEDILLA AND ACUTE
  QTest::newRow("1e0a") << QStringLiteral("\u1e0a") << QByteArray("\xc7" "D"); // LATIN CAPITAL LETTER D WITH DOT ABOVE
  QTest::newRow("1e0b") << QStringLiteral("\u1e0b") << QByteArray("\xc7" "d"); // LATIN SMALL LETTER D WITH DOT ABOVE
  QTest::newRow("1e0c") << QStringLiteral("\u1e0c") << QByteArray("\xd6" "D"); // LATIN CAPITAL LETTER D WITH DOT BELOW
  QTest::newRow("1e0d") << QStringLiteral("\u1e0d") << QByteArray("\xd6" "d"); // LATIN SMALL LETTER D WITH DOT BELOW
  QTest::newRow("1e10") << QStringLiteral("\u1e10") << QByteArray("\xd0" "D"); // LATIN CAPITAL LETTER D WITH CEDILLA
  QTest::newRow("1e11") << QStringLiteral("\u1e11") << QByteArray("\xd0" "d"); // LATIN SMALL LETTER D WITH CEDILLA
  QTest::newRow("1e12") << QStringLiteral("\u1e12") << QByteArray("\xdb" "D"); // LATIN CAPITAL LETTER D WITH CIRCUMFLEX BELOW
  QTest::newRow("1e13") << QStringLiteral("\u1e13") << QByteArray("\xdb" "d"); // LATIN SMALL LETTER D WITH CIRCUMFLEX BELOW
  QTest::newRow("1e14") << QStringLiteral("\u1e14") << QByteArray("\xc1\xc5" "E"); // LATIN CAPITAL LETTER E WITH MACRON AND GRAVE
  QTest::newRow("1e15") << QStringLiteral("\u1e15") << QByteArray("\xc1\xc5" "e"); // LATIN SMALL LETTER E WITH MACRON AND GRAVE
  QTest::newRow("1e16") << QStringLiteral("\u1e16") << QByteArray("\xc2\xc5" "E"); // LATIN CAPITAL LETTER E WITH MACRON AND ACUTE
  QTest::newRow("1e17") << QStringLiteral("\u1e17") << QByteArray("\xc2\xc5" "e"); // LATIN SMALL LETTER E WITH MACRON AND ACUTE
  QTest::newRow("1e18") << QStringLiteral("\u1e18") << QByteArray("\xdb" "E"); // LATIN CAPITAL LETTER E WITH CIRCUMFLEX BELOW
  QTest::newRow("1e19") << QStringLiteral("\u1e19") << QByteArray("\xdb" "e"); // LATIN SMALL LETTER E WITH CIRCUMFLEX BELOW
  QTest::newRow("1e1c") << QStringLiteral("\u1e1c") << QByteArray("\xc6\xd0" "E"); // LATIN CAPITAL LETTER E WITH CEDILLA AND BREVE
  QTest::newRow("1e1d") << QStringLiteral("\u1e1d") << QByteArray("\xc6\xd0" "e"); // LATIN SMALL LETTER E WITH CEDILLA AND BREVE
  QTest::newRow("1e1e") << QStringLiteral("\u1e1e") << QByteArray("\xc7" "F"); // LATIN CAPITAL LETTER F WITH DOT ABOVE
  QTest::newRow("1e1f") << QStringLiteral("\u1e1f") << QByteArray("\xc7" "f"); // LATIN SMALL LETTER F WITH DOT ABOVE
  QTest::newRow("1e20") << QStringLiteral("\u1e20") << QByteArray("\xc5" "G"); // LATIN CAPITAL LETTER G WITH MACRON
  QTest::newRow("1e21") << QStringLiteral("\u1e21") << QByteArray("\xc5" "g"); // LATIN SMALL LETTER G WITH MACRON
  QTest::newRow("1e22") << QStringLiteral("\u1e22") << QByteArray("\xc7" "H"); // LATIN CAPITAL LETTER H WITH DOT ABOVE
  QTest::newRow("1e23") << QStringLiteral("\u1e23") << QByteArray("\xc7" "h"); // LATIN SMALL LETTER H WITH DOT ABOVE
  QTest::newRow("1e24") << QStringLiteral("\u1e24") << QByteArray("\xd6" "H"); // LATIN CAPITAL LETTER H WITH DOT BELOW
  QTest::newRow("1e25") << QStringLiteral("\u1e25") << QByteArray("\xd6" "h"); // LATIN SMALL LETTER H WITH DOT BELOW
  QTest::newRow("1e26") << QStringLiteral("\u1e26") << QByteArray("\xc8" "H"); // LATIN CAPITAL LETTER H WITH DIAERESIS
  QTest::newRow("1e27") << QStringLiteral("\u1e27") << QByteArray("\xc8" "h"); // LATIN SMALL LETTER H WITH DIAERESIS
  QTest::newRow("1e28") << QStringLiteral("\u1e28") << QByteArray("\xd0" "H"); // LATIN CAPITAL LETTER H WITH CEDILLA
  QTest::newRow("1e29") << QStringLiteral("\u1e29") << QByteArray("\xd0" "h"); // LATIN SMALL LETTER H WITH CEDILLA
  QTest::newRow("1e2a") << QStringLiteral("\u1e2a") << QByteArray("\xd5" "H"); // LATIN CAPITAL LETTER H WITH BREVE BELOW
  QTest::newRow("1e2b") << QStringLiteral("\u1e2b") << QByteArray("\xd5" "h"); // LATIN SMALL LETTER H WITH BREVE BELOW
  QTest::newRow("1e2e") << QStringLiteral("\u1e2e") << QByteArray("\xc2\xc8" "I"); // LATIN CAPITAL LETTER I WITH DIAERESIS AND ACUTE
  QTest::newRow("1e2f") << QStringLiteral("\u1e2f") << QByteArray("\xc2\xc8" "i"); // LATIN SMALL LETTER I WITH DIAERESIS AND ACUTE
  QTest::newRow("1e30") << QStringLiteral("\u1e30") << QByteArray("\xc2" "K"); // LATIN CAPITAL LETTER K WITH ACUTE
  QTest::newRow("1e31") << QStringLiteral("\u1e31") << QByteArray("\xc2" "k"); // LATIN SMALL LETTER K WITH ACUTE
  QTest::newRow("1e32") << QStringLiteral("\u1e32") << QByteArray("\xd6" "K"); // LATIN CAPITAL LETTER K WITH DOT BELOW
  QTest::newRow("1e33") << QStringLiteral("\u1e33") << QByteArray("\xd6" "k"); // LATIN SMALL LETTER K WITH DOT BELOW
  QTest::newRow("1e36") << QStringLiteral("\u1e36") << QByteArray("\xd6" "L"); // LATIN CAPITAL LETTER L WITH DOT BELOW
  QTest::newRow("1e37") << QStringLiteral("\u1e37") << QByteArray("\xd6" "l"); // LATIN SMALL LETTER L WITH DOT BELOW
  QTest::newRow("1e38") << QStringLiteral("\u1e38") << QByteArray("\xc5\xd6" "L"); // LATIN CAPITAL LETTER L WITH DOT BELOW AND MACRON
  QTest::newRow("1e39") << QStringLiteral("\u1e39") << QByteArray("\xc5\xd6" "l"); // LATIN SMALL LETTER L WITH DOT BELOW AND MACRON
  QTest::newRow("1e3c") << QStringLiteral("\u1e3c") << QByteArray("\xdb" "L"); // LATIN CAPITAL LETTER L WITH CIRCUMFLEX BELOW
  QTest::newRow("1e3d") << QStringLiteral("\u1e3d") << QByteArray("\xdb" "l"); // LATIN SMALL LETTER L WITH CIRCUMFLEX BELOW
  QTest::newRow("1e3e") << QStringLiteral("\u1e3e") << QByteArray("\xc2" "M"); // LATIN CAPITAL LETTER M WITH ACUTE
  QTest::newRow("1e3f") << QStringLiteral("\u1e3f") << QByteArray("\xc2" "m"); // LATIN SMALL LETTER M WITH ACUTE
  QTest::newRow("1e40") << QStringLiteral("\u1e40") << QByteArray("\xc7" "M"); // LATIN CAPITAL LETTER M WITH DOT ABOVE
  QTest::newRow("1e41") << QStringLiteral("\u1e41") << QByteArray("\xc7" "m"); // LATIN SMALL LETTER M WITH DOT ABOVE
  QTest::newRow("1e42") << QStringLiteral("\u1e42") << QByteArray("\xd6" "M"); // LATIN CAPITAL LETTER M WITH DOT BELOW
  QTest::newRow("1e43") << QStringLiteral("\u1e43") << QByteArray("\xd6" "m"); // LATIN SMALL LETTER M WITH DOT BELOW
  QTest::newRow("1e44") << QStringLiteral("\u1e44") << QByteArray("\xc7" "N"); // LATIN CAPITAL LETTER N WITH DOT ABOVE
  QTest::newRow("1e45") << QStringLiteral("\u1e45") << QByteArray("\xc7" "n"); // LATIN SMALL LETTER N WITH DOT ABOVE
  QTest::newRow("1e46") << QStringLiteral("\u1e46") << QByteArray("\xd6" "N"); // LATIN CAPITAL LETTER N WITH DOT BELOW
  QTest::newRow("1e47") << QStringLiteral("\u1e47") << QByteArray("\xd6" "n"); // LATIN SMALL LETTER N WITH DOT BELOW
  QTest::newRow("1e4a") << QStringLiteral("\u1e4a") << QByteArray("\xdb" "N"); // LATIN CAPITAL LETTER N WITH CIRCUMFLEX BELOW
  QTest::newRow("1e4b") << QStringLiteral("\u1e4b") << QByteArray("\xdb" "n"); // LATIN SMALL LETTER N WITH CIRCUMFLEX BELOW
  QTest::newRow("1e4c") << QStringLiteral("\u1e4c") << QByteArray("\xc2\xc4" "O"); // LATIN CAPITAL LETTER O WITH TILDE AND ACUTE
  QTest::newRow("1e4d") << QStringLiteral("\u1e4d") << QByteArray("\xc2\xc4" "o"); // LATIN SMALL LETTER O WITH TILDE AND ACUTE
  QTest::newRow("1e4e") << QStringLiteral("\u1e4e") << QByteArray("\xc8\xc4" "O"); // LATIN CAPITAL LETTER O WITH TILDE AND DIAERESIS
  QTest::newRow("1e4f") << QStringLiteral("\u1e4f") << QByteArray("\xc8\xc4" "o"); // LATIN SMALL LETTER O WITH TILDE AND DIAERESIS
  QTest::newRow("1e50") << QStringLiteral("\u1e50") << QByteArray("\xc1\xc5" "O"); // LATIN CAPITAL LETTER O WITH MACRON AND GRAVE
  QTest::newRow("1e51") << QStringLiteral("\u1e51") << QByteArray("\xc1\xc5" "o"); // LATIN SMALL LETTER O WITH MACRON AND GRAVE
  QTest::newRow("1e52") << QStringLiteral("\u1e52") << QByteArray("\xc2\xc5" "O"); // LATIN CAPITAL LETTER O WITH MACRON AND ACUTE
  QTest::newRow("1e53") << QStringLiteral("\u1e53") << QByteArray("\xc2\xc5" "o"); // LATIN SMALL LETTER O WITH MACRON AND ACUTE
  QTest::newRow("1e54") << QStringLiteral("\u1e54") << QByteArray("\xc2" "P"); // LATIN CAPITAL LETTER P WITH ACUTE
  QTest::newRow("1e55") << QStringLiteral("\u1e55") << QByteArray("\xc2" "p"); // LATIN SMALL LETTER P WITH ACUTE
  QTest::newRow("1e56") << QStringLiteral("\u1e56") << QByteArray("\xc7" "P"); // LATIN CAPITAL LETTER P WITH DOT ABOVE
  QTest::newRow("1e57") << QStringLiteral("\u1e57") << QByteArray("\xc7" "p"); // LATIN SMALL LETTER P WITH DOT ABOVE
  QTest::newRow("1e58") << QStringLiteral("\u1e58") << QByteArray("\xc7" "R"); // LATIN CAPITAL LETTER R WITH DOT ABOVE
  QTest::newRow("1e59") << QStringLiteral("\u1e59") << QByteArray("\xc7" "r"); // LATIN SMALL LETTER R WITH DOT ABOVE
  QTest::newRow("1e5a") << QStringLiteral("\u1e5a") << QByteArray("\xd6" "R"); // LATIN CAPITAL LETTER R WITH DOT BELOW
  QTest::newRow("1e5b") << QStringLiteral("\u1e5b") << QByteArray("\xd6" "r"); // LATIN SMALL LETTER R WITH DOT BELOW
  QTest::newRow("1e5c") << QStringLiteral("\u1e5c") << QByteArray("\xc5\xd6" "R"); // LATIN CAPITAL LETTER R WITH DOT BELOW AND MACRON
  QTest::newRow("1e5d") << QStringLiteral("\u1e5d") << QByteArray("\xc5\xd6" "r"); // LATIN SMALL LETTER R WITH DOT BELOW AND MACRON
  QTest::newRow("1e60") << QStringLiteral("\u1e60") << QByteArray("\xc7" "S"); // LATIN CAPITAL LETTER S WITH DOT ABOVE
  QTest::newRow("1e61") << QStringLiteral("\u1e61") << QByteArray("\xc7" "s"); // LATIN SMALL LETTER S WITH DOT ABOVE
  QTest::newRow("1e62") << QStringLiteral("\u1e62") << QByteArray("\xd6" "S"); // LATIN CAPITAL LETTER S WITH DOT BELOW
  QTest::newRow("1e63") << QStringLiteral("\u1e63") << QByteArray("\xd6" "s"); // LATIN SMALL LETTER S WITH DOT BELOW
  QTest::newRow("1e64") << QStringLiteral("\u1e64") << QByteArray("\xc7\xc2" "S"); // LATIN CAPITAL LETTER S WITH ACUTE AND DOT ABOVE
  QTest::newRow("1e65") << QStringLiteral("\u1e65") << QByteArray("\xc7\xc2" "s"); // LATIN SMALL LETTER S WITH ACUTE AND DOT ABOVE
  QTest::newRow("1e66") << QStringLiteral("\u1e66") << QByteArray("\xc7\xcf" "S"); // LATIN CAPITAL LETTER S WITH CARON AND DOT ABOVE
  QTest::newRow("1e67") << QStringLiteral("\u1e67") << QByteArray("\xc7\xcf" "s"); // LATIN SMALL LETTER S WITH CARON AND DOT ABOVE
  QTest::newRow("1e68") << QStringLiteral("\u1e68") << QByteArray("\xc7\xd6" "S"); // LATIN CAPITAL LETTER S WITH DOT BELOW AND DOT ABOVE
  QTest::newRow("1e69") << QStringLiteral("\u1e69") << QByteArray("\xc7\xd6" "s"); // LATIN SMALL LETTER S WITH DOT BELOW AND DOT ABOVE
  QTest::newRow("1e6a") << QStringLiteral("\u1e6a") << QByteArray("\xc7" "T"); // LATIN CAPITAL LETTER T WITH DOT ABOVE
  QTest::newRow("1e6b") << QStringLiteral("\u1e6b") << QByteArray("\xc7" "t"); // LATIN SMALL LETTER T WITH DOT ABOVE
  QTest::newRow("1e6c") << QStringLiteral("\u1e6c") << QByteArray("\xd6" "T"); // LATIN CAPITAL LETTER T WITH DOT BELOW
  QTest::newRow("1e6d") << QStringLiteral("\u1e6d") << QByteArray("\xd6" "t"); // LATIN SMALL LETTER T WITH DOT BELOW
  QTest::newRow("1e70") << QStringLiteral("\u1e70") << QByteArray("\xdb" "T"); // LATIN CAPITAL LETTER T WITH CIRCUMFLEX BELOW
  QTest::newRow("1e71") << QStringLiteral("\u1e71") << QByteArray("\xdb" "t"); // LATIN SMALL LETTER T WITH CIRCUMFLEX BELOW
  QTest::newRow("1e72") << QStringLiteral("\u1e72") << QByteArray("\xd7" "U"); // LATIN CAPITAL LETTER U WITH DIAERESIS BELOW
  QTest::newRow("1e73") << QStringLiteral("\u1e73") << QByteArray("\xd7" "u"); // LATIN SMALL LETTER U WITH DIAERESIS BELOW
  QTest::newRow("1e76") << QStringLiteral("\u1e76") << QByteArray("\xdb" "U"); // LATIN CAPITAL LETTER U WITH CIRCUMFLEX BELOW
  QTest::newRow("1e77") << QStringLiteral("\u1e77") << QByteArray("\xdb" "u"); // LATIN SMALL LETTER U WITH CIRCUMFLEX BELOW
  QTest::newRow("1e78") << QStringLiteral("\u1e78") << QByteArray("\xc2\xc4" "U"); // LATIN CAPITAL LETTER U WITH TILDE AND ACUTE
  QTest::newRow("1e79") << QStringLiteral("\u1e79") << QByteArray("\xc2\xc4" "u"); // LATIN SMALL LETTER U WITH TILDE AND ACUTE
  QTest::newRow("1e7a") << QStringLiteral("\u1e7a") << QByteArray("\xc8\xc5" "U"); // LATIN CAPITAL LETTER U WITH MACRON AND DIAERESIS
  QTest::newRow("1e7b") << QStringLiteral("\u1e7b") << QByteArray("\xc8\xc5" "u"); // LATIN SMALL LETTER U WITH MACRON AND DIAERESIS
  QTest::newRow("1e7c") << QStringLiteral("\u1e7c") << QByteArray("\xc4" "V"); // LATIN CAPITAL LETTER V WITH TILDE
  QTest::newRow("1e7d") << QStringLiteral("\u1e7d") << QByteArray("\xc4" "v"); // LATIN SMALL LETTER V WITH TILDE
  QTest::newRow("1e7e") << QStringLiteral("\u1e7e") << QByteArray("\xd6" "V"); // LATIN CAPITAL LETTER V WITH DOT BELOW
  QTest::newRow("1e7f") << QStringLiteral("\u1e7f") << QByteArray("\xd6" "v"); // LATIN SMALL LETTER V WITH DOT BELOW
  QTest::newRow("1e80") << QStringLiteral("\u1e80") << QByteArray("\xc1" "W"); // LATIN CAPITAL LETTER W WITH GRAVE
  QTest::newRow("1e81") << QStringLiteral("\u1e81") << QByteArray("\xc1" "w"); // LATIN SMALL LETTER W WITH GRAVE
  QTest::newRow("1e82") << QStringLiteral("\u1e82") << QByteArray("\xc2" "W"); // LATIN CAPITAL LETTER W WITH ACUTE
  QTest::newRow("1e83") << QStringLiteral("\u1e83") << QByteArray("\xc2" "w"); // LATIN SMALL LETTER W WITH ACUTE
  QTest::newRow("1e84") << QStringLiteral("\u1e84") << QByteArray("\xc8" "W"); // LATIN CAPITAL LETTER W WITH DIAERESIS
  QTest::newRow("1e85") << QStringLiteral("\u1e85") << QByteArray("\xc8" "w"); // LATIN SMALL LETTER W WITH DIAERESIS
  QTest::newRow("1e86") << QStringLiteral("\u1e86") << QByteArray("\xc7" "W"); // LATIN CAPITAL LETTER W WITH DOT ABOVE
  QTest::newRow("1e87") << QStringLiteral("\u1e87") << QByteArray("\xc7" "w"); // LATIN SMALL LETTER W WITH DOT ABOVE
  QTest::newRow("1e88") << QStringLiteral("\u1e88") << QByteArray("\xd6" "W"); // LATIN CAPITAL LETTER W WITH DOT BELOW
  QTest::newRow("1e89") << QStringLiteral("\u1e89") << QByteArray("\xd6" "w"); // LATIN SMALL LETTER W WITH DOT BELOW
  QTest::newRow("1e8a") << QStringLiteral("\u1e8a") << QByteArray("\xc7" "X"); // LATIN CAPITAL LETTER X WITH DOT ABOVE
  QTest::newRow("1e8b") << QStringLiteral("\u1e8b") << QByteArray("\xc7" "x"); // LATIN SMALL LETTER X WITH DOT ABOVE
  QTest::newRow("1e8c") << QStringLiteral("\u1e8c") << QByteArray("\xc8" "X"); // LATIN CAPITAL LETTER X WITH DIAERESIS
  QTest::newRow("1e8d") << QStringLiteral("\u1e8d") << QByteArray("\xc8" "x"); // LATIN SMALL LETTER X WITH DIAERESIS
  QTest::newRow("1e8e") << QStringLiteral("\u1e8e") << QByteArray("\xc7" "Y"); // LATIN CAPITAL LETTER Y WITH DOT ABOVE
  QTest::newRow("1e8f") << QStringLiteral("\u1e8f") << QByteArray("\xc7" "y"); // LATIN SMALL LETTER Y WITH DOT ABOVE
  QTest::newRow("1e90") << QStringLiteral("\u1e90") << QByteArray("\xc3" "Z"); // LATIN CAPITAL LETTER Z WITH CIRCUMFLEX
  QTest::newRow("1e91") << QStringLiteral("\u1e91") << QByteArray("\xc3" "z"); // LATIN SMALL LETTER Z WITH CIRCUMFLEX
  QTest::newRow("1e92") << QStringLiteral("\u1e92") << QByteArray("\xd6" "Z"); // LATIN CAPITAL LETTER Z WITH DOT BELOW
  QTest::newRow("1e93") << QStringLiteral("\u1e93") << QByteArray("\xd6" "z"); // LATIN SMALL LETTER Z WITH DOT BELOW
  QTest::newRow("1e97") << QStringLiteral("\u1e97") << QByteArray("\xc8" "t"); // LATIN SMALL LETTER T WITH DIAERESIS
  QTest::newRow("1e98") << QStringLiteral("\u1e98") << QByteArray("\xca" "w"); // LATIN SMALL LETTER W WITH RING ABOVE
  QTest::newRow("1e99") << QStringLiteral("\u1e99") << QByteArray("\xca" "y"); // LATIN SMALL LETTER Y WITH RING ABOVE
  QTest::newRow("1ea0") << QStringLiteral("\u1ea0") << QByteArray("\xd6" "A"); // LATIN CAPITAL LETTER A WITH DOT BELOW
  QTest::newRow("1ea1") << QStringLiteral("\u1ea1") << QByteArray("\xd6" "a"); // LATIN SMALL LETTER A WITH DOT BELOW
  QTest::newRow("1ea2") << QStringLiteral("\u1ea2") << QByteArray("\xc0" "A"); // LATIN CAPITAL LETTER A WITH HOOK ABOVE
  QTest::newRow("1ea3") << QStringLiteral("\u1ea3") << QByteArray("\xc0" "a"); // LATIN SMALL LETTER A WITH HOOK ABOVE
  QTest::newRow("1ea4") << QStringLiteral("\u1ea4") << QByteArray("\xc2\xc3" "A"); // LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND ACUTE
  QTest::newRow("1ea5") << QStringLiteral("\u1ea5") << QByteArray("\xc2\xc3" "a"); // LATIN SMALL LETTER A WITH CIRCUMFLEX AND ACUTE
  QTest::newRow("1ea6") << QStringLiteral("\u1ea6") << QByteArray("\xc1\xc3" "A"); // LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND GRAVE
  QTest::newRow("1ea7") << QStringLiteral("\u1ea7") << QByteArray("\xc1\xc3" "a"); // LATIN SMALL LETTER A WITH CIRCUMFLEX AND GRAVE
  QTest::newRow("1ea8") << QStringLiteral("\u1ea8") << QByteArray("\xc0\xc3" "A"); // LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE
  QTest::newRow("1ea9") << QStringLiteral("\u1ea9") << QByteArray("\xc0\xc3" "a"); // LATIN SMALL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE
  QTest::newRow("1eaa") << QStringLiteral("\u1eaa") << QByteArray("\xc4\xc3" "A"); // LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND TILDE
  QTest::newRow("1eab") << QStringLiteral("\u1eab") << QByteArray("\xc4\xc3" "a"); // LATIN SMALL LETTER A WITH CIRCUMFLEX AND TILDE
  QTest::newRow("1eac") << QStringLiteral("\u1eac") << QByteArray("\xc3\xd6" "A"); // LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND DOT BELOW
  QTest::newRow("1ead") << QStringLiteral("\u1ead") << QByteArray("\xc3\xd6" "a"); // LATIN SMALL LETTER A WITH CIRCUMFLEX AND DOT BELOW
  QTest::newRow("1eae") << QStringLiteral("\u1eae") << QByteArray("\xc2\xc6" "A"); // LATIN CAPITAL LETTER A WITH BREVE AND ACUTE
  QTest::newRow("1eaf") << QStringLiteral("\u1eaf") << QByteArray("\xc2\xc6" "a"); // LATIN SMALL LETTER A WITH BREVE AND ACUTE
  QTest::newRow("1eb0") << QStringLiteral("\u1eb0") << QByteArray("\xc1\xc6" "A"); // LATIN CAPITAL LETTER A WITH BREVE AND GRAVE
  QTest::newRow("1eb1") << QStringLiteral("\u1eb1") << QByteArray("\xc1\xc6" "a"); // LATIN SMALL LETTER A WITH BREVE AND GRAVE
  QTest::newRow("1eb2") << QStringLiteral("\u1eb2") << QByteArray("\xc0\xc6" "A"); // LATIN CAPITAL LETTER A WITH BREVE AND HOOK ABOVE
  QTest::newRow("1eb3") << QStringLiteral("\u1eb3") << QByteArray("\xc0\xc6" "a"); // LATIN SMALL LETTER A WITH BREVE AND HOOK ABOVE
  QTest::newRow("1eb4") << QStringLiteral("\u1eb4") << QByteArray("\xc4\xc6" "A"); // LATIN CAPITAL LETTER A WITH BREVE AND TILDE
  QTest::newRow("1eb5") << QStringLiteral("\u1eb5") << QByteArray("\xc4\xc6" "a"); // LATIN SMALL LETTER A WITH BREVE AND TILDE
  QTest::newRow("1eb6") << QStringLiteral("\u1eb6") << QByteArray("\xc6\xd6" "A"); // LATIN CAPITAL LETTER A WITH BREVE AND DOT BELOW
  QTest::newRow("1eb7") << QStringLiteral("\u1eb7") << QByteArray("\xc6\xd6" "a"); // LATIN SMALL LETTER A WITH BREVE AND DOT BELOW
  QTest::newRow("1eb8") << QStringLiteral("\u1eb8") << QByteArray("\xd6" "E"); // LATIN CAPITAL LETTER E WITH DOT BELOW
  QTest::newRow("1eb9") << QStringLiteral("\u1eb9") << QByteArray("\xd6" "e"); // LATIN SMALL LETTER E WITH DOT BELOW
  QTest::newRow("1eba") << QStringLiteral("\u1eba") << QByteArray("\xc0" "E"); // LATIN CAPITAL LETTER E WITH HOOK ABOVE
  QTest::newRow("1ebb") << QStringLiteral("\u1ebb") << QByteArray("\xc0" "e"); // LATIN SMALL LETTER E WITH HOOK ABOVE
  QTest::newRow("1ebc") << QStringLiteral("\u1ebc") << QByteArray("\xc4" "E"); // LATIN CAPITAL LETTER E WITH TILDE
  QTest::newRow("1ebd") << QStringLiteral("\u1ebd") << QByteArray("\xc4" "e"); // LATIN SMALL LETTER E WITH TILDE
  QTest::newRow("1ebe") << QStringLiteral("\u1ebe") << QByteArray("\xc2\xc3" "E"); // LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND ACUTE
  QTest::newRow("1ebf") << QStringLiteral("\u1ebf") << QByteArray("\xc2\xc3" "e"); // LATIN SMALL LETTER E WITH CIRCUMFLEX AND ACUTE
  QTest::newRow("1ec0") << QStringLiteral("\u1ec0") << QByteArray("\xc1\xc3" "E"); // LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND GRAVE
  QTest::newRow("1ec1") << QStringLiteral("\u1ec1") << QByteArray("\xc1\xc3" "e"); // LATIN SMALL LETTER E WITH CIRCUMFLEX AND GRAVE
  QTest::newRow("1ec2") << QStringLiteral("\u1ec2") << QByteArray("\xc0\xc3" "E"); // LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE
  QTest::newRow("1ec3") << QStringLiteral("\u1ec3") << QByteArray("\xc0\xc3" "e"); // LATIN SMALL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE
  QTest::newRow("1ec4") << QStringLiteral("\u1ec4") << QByteArray("\xc4\xc3" "E"); // LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND TILDE
  QTest::newRow("1ec5") << QStringLiteral("\u1ec5") << QByteArray("\xc4\xc3" "e"); // LATIN SMALL LETTER E WITH CIRCUMFLEX AND TILDE
  QTest::newRow("1ec6") << QStringLiteral("\u1ec6") << QByteArray("\xc3\xd6" "E"); // LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND DOT BELOW
  QTest::newRow("1ec7") << QStringLiteral("\u1ec7") << QByteArray("\xc3\xd6" "e"); // LATIN SMALL LETTER E WITH CIRCUMFLEX AND DOT BELOW
  QTest::newRow("1ec8") << QStringLiteral("\u1ec8") << QByteArray("\xc0" "I"); // LATIN CAPITAL LETTER I WITH HOOK ABOVE
  QTest::newRow("1ec9") << QStringLiteral("\u1ec9") << QByteArray("\xc0" "i"); // LATIN SMALL LETTER I WITH HOOK ABOVE
  QTest::newRow("1eca") << QStringLiteral("\u1eca") << QByteArray("\xd6" "I"); // LATIN CAPITAL LETTER I WITH DOT BELOW
  QTest::newRow("1ecb") << QStringLiteral("\u1ecb") << QByteArray("\xd6" "i"); // LATIN SMALL LETTER I WITH DOT BELOW
  QTest::newRow("1ecc") << QStringLiteral("\u1ecc") << QByteArray("\xd6" "O"); // LATIN CAPITAL LETTER O WITH DOT BELOW
  QTest::newRow("1ecd") << QStringLiteral("\u1ecd") << QByteArray("\xd6" "o"); // LATIN SMALL LETTER O WITH DOT BELOW
  QTest::newRow("1ece") << QStringLiteral("\u1ece") << QByteArray("\xc0" "O"); // LATIN CAPITAL LETTER O WITH HOOK ABOVE
  QTest::newRow("1ecf") << QStringLiteral("\u1ecf") << QByteArray("\xc0" "o"); // LATIN SMALL LETTER O WITH HOOK ABOVE
  QTest::newRow("1ed0") << QStringLiteral("\u1ed0") << QByteArray("\xc2\xc3" "O"); // LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND ACUTE
  QTest::newRow("1ed1") << QStringLiteral("\u1ed1") << QByteArray("\xc2\xc3" "o"); // LATIN SMALL LETTER O WITH CIRCUMFLEX AND ACUTE
  QTest::newRow("1ed2") << QStringLiteral("\u1ed2") << QByteArray("\xc1\xc3" "O"); // LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND GRAVE
  QTest::newRow("1ed3") << QStringLiteral("\u1ed3") << QByteArray("\xc1\xc3" "o"); // LATIN SMALL LETTER O WITH CIRCUMFLEX AND GRAVE
  QTest::newRow("1ed4") << QStringLiteral("\u1ed4") << QByteArray("\xc0\xc3" "O"); // LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE
  QTest::newRow("1ed5") << QStringLiteral("\u1ed5") << QByteArray("\xc0\xc3" "o"); // LATIN SMALL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE
  QTest::newRow("1ed6") << QStringLiteral("\u1ed6") << QByteArray("\xc4\xc3" "O"); // LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND TILDE
  QTest::newRow("1ed7") << QStringLiteral("\u1ed7") << QByteArray("\xc4\xc3" "o"); // LATIN SMALL LETTER O WITH CIRCUMFLEX AND TILDE
  QTest::newRow("1ed8") << QStringLiteral("\u1ed8") << QByteArray("\xc3\xd6" "O"); // LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND DOT BELOW
  QTest::newRow("1ed9") << QStringLiteral("\u1ed9") << QByteArray("\xc3\xd6" "o"); // LATIN SMALL LETTER O WITH CIRCUMFLEX AND DOT BELOW
  QTest::newRow("1eda") << QStringLiteral("\u1eda") << QByteArray("\xc2\xce" "O"); // LATIN CAPITAL LETTER O WITH HORN AND ACUTE
  QTest::newRow("1edb") << QStringLiteral("\u1edb") << QByteArray("\xc2\xce" "o"); // LATIN SMALL LETTER O WITH HORN AND ACUTE
  QTest::newRow("1edc") << QStringLiteral("\u1edc") << QByteArray("\xc1\xce" "O"); // LATIN CAPITAL LETTER O WITH HORN AND GRAVE
  QTest::newRow("1edd") << QStringLiteral("\u1edd") << QByteArray("\xc1\xce" "o"); // LATIN SMALL LETTER O WITH HORN AND GRAVE
  QTest::newRow("1ede") << QStringLiteral("\u1ede") << QByteArray("\xc0\xce" "O"); // LATIN CAPITAL LETTER O WITH HORN AND HOOK ABOVE
  QTest::newRow("1edf") << QStringLiteral("\u1edf") << QByteArray("\xc0\xce" "o"); // LATIN SMALL LETTER O WITH HORN AND HOOK ABOVE
  QTest::newRow("1ee0") << QStringLiteral("\u1ee0") << QByteArray("\xc4\xce" "O"); // LATIN CAPITAL LETTER O WITH HORN AND TILDE
  QTest::newRow("1ee1") << QStringLiteral("\u1ee1") << QByteArray("\xc4\xce" "o"); // LATIN SMALL LETTER O WITH HORN AND TILDE
  QTest::newRow("1ee2") << QStringLiteral("\u1ee2") << QByteArray("\xd6\xce" "O"); // LATIN CAPITAL LETTER O WITH HORN AND DOT BELOW
  QTest::newRow("1ee3") << QStringLiteral("\u1ee3") << QByteArray("\xd6\xce" "o"); // LATIN SMALL LETTER O WITH HORN AND DOT BELOW
  QTest::newRow("1ee4") << QStringLiteral("\u1ee4") << QByteArray("\xd6" "U"); // LATIN CAPITAL LETTER U WITH DOT BELOW
  QTest::newRow("1ee5") << QStringLiteral("\u1ee5") << QByteArray("\xd6" "u"); // LATIN SMALL LETTER U WITH DOT BELOW
  QTest::newRow("1ee6") << QStringLiteral("\u1ee6") << QByteArray("\xc0" "U"); // LATIN CAPITAL LETTER U WITH HOOK ABOVE
  QTest::newRow("1ee7") << QStringLiteral("\u1ee7") << QByteArray("\xc0" "u"); // LATIN SMALL LETTER U WITH HOOK ABOVE
  QTest::newRow("1ee8") << QStringLiteral("\u1ee8") << QByteArray("\xc2\xce" "U"); // LATIN CAPITAL LETTER U WITH HORN AND ACUTE
  QTest::newRow("1ee9") << QStringLiteral("\u1ee9") << QByteArray("\xc2\xce" "u"); // LATIN SMALL LETTER U WITH HORN AND ACUTE
  QTest::newRow("1eea") << QStringLiteral("\u1eea") << QByteArray("\xc1\xce" "U"); // LATIN CAPITAL LETTER U WITH HORN AND GRAVE
  QTest::newRow("1eeb") << QStringLiteral("\u1eeb") << QByteArray("\xc1\xce" "u"); // LATIN SMALL LETTER U WITH HORN AND GRAVE
  QTest::newRow("1eec") << QStringLiteral("\u1eec") << QByteArray("\xc0\xce" "U"); // LATIN CAPITAL LETTER U WITH HORN AND HOOK ABOVE
  QTest::newRow("1eed") << QStringLiteral("\u1eed") << QByteArray("\xc0\xce" "u"); // LATIN SMALL LETTER U WITH HORN AND HOOK ABOVE
  QTest::newRow("1eee") << QStringLiteral("\u1eee") << QByteArray("\xc4\xce" "U"); // LATIN CAPITAL LETTER U WITH HORN AND TILDE
  QTest::newRow("1eef") << QStringLiteral("\u1eef") << QByteArray("\xc4\xce" "u"); // LATIN SMALL LETTER U WITH HORN AND TILDE
  QTest::newRow("1ef0") << QStringLiteral("\u1ef0") << QByteArray("\xd6\xce" "U"); // LATIN CAPITAL LETTER U WITH HORN AND DOT BELOW
  QTest::newRow("1ef1") << QStringLiteral("\u1ef1") << QByteArray("\xd6\xce" "u"); // LATIN SMALL LETTER U WITH HORN AND DOT BELOW
  QTest::newRow("1ef2") << QStringLiteral("\u1ef2") << QByteArray("\xc1" "Y"); // LATIN CAPITAL LETTER Y WITH GRAVE
  QTest::newRow("1ef3") << QStringLiteral("\u1ef3") << QByteArray("\xc1" "y"); // LATIN SMALL LETTER Y WITH GRAVE
  QTest::newRow("1ef4") << QStringLiteral("\u1ef4") << QByteArray("\xd6" "Y"); // LATIN CAPITAL LETTER Y WITH DOT BELOW
  QTest::newRow("1ef5") << QStringLiteral("\u1ef5") << QByteArray("\xd6" "y"); // LATIN SMALL LETTER Y WITH DOT BELOW
  QTest::newRow("1ef6") << QStringLiteral("\u1ef6") << QByteArray("\xc0" "Y"); // LATIN CAPITAL LETTER Y WITH HOOK ABOVE
  QTest::newRow("1ef7") << QStringLiteral("\u1ef7") << QByteArray("\xc0" "y"); // LATIN SMALL LETTER Y WITH HOOK ABOVE
  QTest::newRow("1ef8") << QStringLiteral("\u1ef8") << QByteArray("\xc4" "Y"); // LATIN CAPITAL LETTER Y WITH TILDE
  QTest::newRow("1ef9") << QStringLiteral("\u1ef9") << QByteArray("\xc4" "y"); // LATIN SMALL LETTER Y WITH TILDE
//  QTest::newRow("1fef") << QStringLiteral("\u1fef") << QByteArray("`"); // GREEK VARIA
  QTest::newRow("2018") << QStringLiteral("\u2018") << QByteArray("\xa9"); // LEFT SINGLE QUOTATION MARK
  QTest::newRow("2019") << QStringLiteral("\u2019") << QByteArray("\xb9"); // RIGHT SINGLE QUOTATION MARK
  QTest::newRow("201a") << QStringLiteral("\u201a") << QByteArray("\xb2"); // SINGLE LOW-9 QUOTATION MARK
  QTest::newRow("201c") << QStringLiteral("\u201c") << QByteArray("\xaa"); // LEFT DOUBLE QUOTATION MARK
  QTest::newRow("201d") << QStringLiteral("\u201d") << QByteArray("\xba"); // RIGHT DOUBLE QUOTATION MARK
  QTest::newRow("201e") << QStringLiteral("\u201e") << QByteArray("\xa2"); // DOUBLE LOW-9 QUOTATION MARK
  QTest::newRow("2020") << QStringLiteral("\u2020") << QByteArray("\xa6"); // DAGGER
  QTest::newRow("2021") << QStringLiteral("\u2021") << QByteArray("\xb6"); // DOUBLE DAGGER
  QTest::newRow("2032") << QStringLiteral("\u2032") << QByteArray("\xa8"); // PRIME
  QTest::newRow("2033") << QStringLiteral("\u2033") << QByteArray("\xb8"); // DOUBLE PRIME
  QTest::newRow("2117") << QStringLiteral("\u2117") << QByteArray("\xae"); // SOUND RECORDING COPYRIGHT
//  QTest::newRow("212b") << QStringLiteral("\u212b") << QByteArray("\xca" "A"); // ANGSTROM SIGN
  QTest::newRow("266d") << QStringLiteral("\u266d") << QByteArray("\xac"); // MUSIC FLAT SIGN
  QTest::newRow("266f") << QStringLiteral("\u266f") << QByteArray("\xbc"); // MUSIC SHARP SIGN
//  QTest::newRow("fe20") << QStringLiteral("\ufe20") << QByteArray("\xdd"); // COMBINING LIGATURE LEFT HALF
//  QTest::newRow("fe21") << QStringLiteral("\ufe21") << QByteArray("\xde"); // COMBINING LIGATURE RIGHT HALF
//  QTest::newRow("fe23") << QStringLiteral("\ufe23") << QByteArray("\xdf"); // COMBINING DOUBLE TILDE RIGHT HALF
}
