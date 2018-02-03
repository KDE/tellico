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

#include "iso6937test.h"
#include "../utils/iso6937converter.h"

#include <QTest>

#define QL1(x) QStringLiteral(x)
#define QU8(x) QString::fromUtf8(x)

QTEST_APPLESS_MAIN( Iso6937Test )

void Iso6937Test::testAscii() {
  QFETCH(QByteArray, input);
  QFETCH(QString, output);

  QCOMPARE(Tellico::Iso6937Converter::toUtf8(input), output);
}

// https://github.com/noophq/java-charset/blob/master/src/test/java/fr/noop/charset/iso6937/Iso6937CharsetDecoderTest.java
void Iso6937Test::testAscii_data() {
  QTest::addColumn<QByteArray>("input");
  QTest::addColumn<QString>("output");

  QTest::newRow("lowercase") << QByteArray("abcdefghijklmnopqrstuvwxyz") << QL1("abcdefghijklmnopqrstuvwxyz");
  QTest::newRow("uppercase") << QByteArray("ABCDEFGHIJKLMNOPQRSTUVWXYZ") << QL1("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  QTest::newRow("numbers") << QByteArray("1234567890") << QL1("1234567890");
  QTest::newRow("symbols") << QByteArray("`-=[]\\;',./~!@#$%^&*()_+{}|:\"<>?") << QL1("`-=[]\\;',./~!@#$%^&*()_+{}|:\"<>?");
}

void Iso6937Test::testAccent() {
  QFETCH(QString, output);
  QFETCH(QByteArray, input);

  QCOMPARE(Tellico::Iso6937Converter::toUtf8(input), output);
}

// https://github.com/noophq/java-charset/blob/master/src/test/java/fr/noop/charset/iso6937/Iso6937CharsetDecoderTest.java
void Iso6937Test::testAccent_data() {
  QTest::addColumn<QString>("output");
  QTest::addColumn<QByteArray>("input");

  QTest::newRow("c141") << QU8("À") << QByteArray::fromHex("c141");
  QTest::newRow("c145") << QU8("È") << QByteArray::fromHex("c145");
  QTest::newRow("c149") << QU8("Ì") << QByteArray::fromHex("c149");
  QTest::newRow("c14f") << QU8("Ò") << QByteArray::fromHex("c14f");
  QTest::newRow("c155") << QU8("Ù") << QByteArray::fromHex("c155");
  QTest::newRow("c161") << QU8("à") << QByteArray::fromHex("c161");
  QTest::newRow("c165") << QU8("è") << QByteArray::fromHex("c165");
  QTest::newRow("c169") << QU8("ì") << QByteArray::fromHex("c169");
  QTest::newRow("c16f") << QU8("ò") << QByteArray::fromHex("c16f");
  QTest::newRow("c175") << QU8("ù") << QByteArray::fromHex("c175");
  QTest::newRow("c241") << QU8("Á") << QByteArray::fromHex("c241");
  QTest::newRow("c243") << QU8("Ć") << QByteArray::fromHex("c243");
  QTest::newRow("c245") << QU8("É") << QByteArray::fromHex("c245");
  QTest::newRow("c249") << QU8("Í") << QByteArray::fromHex("c249");
  QTest::newRow("c24c") << QU8("Ĺ") << QByteArray::fromHex("c24c");
  QTest::newRow("c24e") << QU8("Ń") << QByteArray::fromHex("c24e");
  QTest::newRow("c24f") << QU8("Ó") << QByteArray::fromHex("c24f");
  QTest::newRow("c252") << QU8("Ŕ") << QByteArray::fromHex("c252");
  QTest::newRow("c253") << QU8("Ś") << QByteArray::fromHex("c253");
  QTest::newRow("c255") << QU8("Ú") << QByteArray::fromHex("c255");
  QTest::newRow("c259") << QU8("Ý") << QByteArray::fromHex("c259");
  QTest::newRow("c25a") << QU8("Ź") << QByteArray::fromHex("c25a");
  QTest::newRow("c261") << QU8("á") << QByteArray::fromHex("c261");
  QTest::newRow("c263") << QU8("ć") << QByteArray::fromHex("c263");
  QTest::newRow("c265") << QU8("é") << QByteArray::fromHex("c265");
  QTest::newRow("c269") << QU8("í") << QByteArray::fromHex("c269");
  QTest::newRow("c26c") << QU8("ĺ") << QByteArray::fromHex("c26c");
  QTest::newRow("c26e") << QU8("ń") << QByteArray::fromHex("c26e");
  QTest::newRow("c26f") << QU8("ó") << QByteArray::fromHex("c26f");
  QTest::newRow("c272") << QU8("ŕ") << QByteArray::fromHex("c272");
  QTest::newRow("c273") << QU8("ś") << QByteArray::fromHex("c273");
  QTest::newRow("c275") << QU8("ú") << QByteArray::fromHex("c275");
  QTest::newRow("c279") << QU8("ý") << QByteArray::fromHex("c279");
  QTest::newRow("c27a") << QU8("ź") << QByteArray::fromHex("c27a");
  QTest::newRow("c341") << QU8("Â") << QByteArray::fromHex("c341");
  QTest::newRow("c343") << QU8("Ĉ") << QByteArray::fromHex("c343");
  QTest::newRow("c345") << QU8("Ê") << QByteArray::fromHex("c345");
  QTest::newRow("c347") << QU8("Ĝ") << QByteArray::fromHex("c347");
  QTest::newRow("c348") << QU8("Ĥ") << QByteArray::fromHex("c348");
  QTest::newRow("c349") << QU8("Î") << QByteArray::fromHex("c349");
  QTest::newRow("c34a") << QU8("Ĵ") << QByteArray::fromHex("c34a");
  QTest::newRow("c34f") << QU8("Ô") << QByteArray::fromHex("c34f");
  QTest::newRow("c353") << QU8("Ŝ") << QByteArray::fromHex("c353");
  QTest::newRow("c355") << QU8("Û") << QByteArray::fromHex("c355");
  QTest::newRow("c357") << QU8("Ŵ") << QByteArray::fromHex("c357");
  QTest::newRow("c359") << QU8("Ŷ") << QByteArray::fromHex("c359");
  QTest::newRow("c361") << QU8("â") << QByteArray::fromHex("c361");
  QTest::newRow("c363") << QU8("ĉ") << QByteArray::fromHex("c363");
  QTest::newRow("c365") << QU8("ê") << QByteArray::fromHex("c365");
  QTest::newRow("c367") << QU8("ĝ") << QByteArray::fromHex("c367");
  QTest::newRow("c368") << QU8("ĥ") << QByteArray::fromHex("c368");
  QTest::newRow("c369") << QU8("î") << QByteArray::fromHex("c369");
  QTest::newRow("c36a") << QU8("ĵ") << QByteArray::fromHex("c36a");
  QTest::newRow("c36f") << QU8("ô") << QByteArray::fromHex("c36f");
  QTest::newRow("c373") << QU8("ŝ") << QByteArray::fromHex("c373");
  QTest::newRow("c375") << QU8("û") << QByteArray::fromHex("c375");
  QTest::newRow("c377") << QU8("ŵ") << QByteArray::fromHex("c377");
  QTest::newRow("c379") << QU8("ŷ") << QByteArray::fromHex("c379");
  QTest::newRow("c441") << QU8("Ã") << QByteArray::fromHex("c441");
  QTest::newRow("c449") << QU8("Ĩ") << QByteArray::fromHex("c449");
  QTest::newRow("c44e") << QU8("Ñ") << QByteArray::fromHex("c44e");
  QTest::newRow("c44f") << QU8("Õ") << QByteArray::fromHex("c44f");
  QTest::newRow("c455") << QU8("Ũ") << QByteArray::fromHex("c455");
  QTest::newRow("c461") << QU8("ã") << QByteArray::fromHex("c461");
  QTest::newRow("c469") << QU8("ĩ") << QByteArray::fromHex("c469");
  QTest::newRow("c46e") << QU8("ñ") << QByteArray::fromHex("c46e");
  QTest::newRow("c46f") << QU8("õ") << QByteArray::fromHex("c46f");
  QTest::newRow("c475") << QU8("ũ") << QByteArray::fromHex("c475");
  QTest::newRow("c541") << QU8("Ā") << QByteArray::fromHex("c541");
  QTest::newRow("c545") << QU8("Ē") << QByteArray::fromHex("c545");
  QTest::newRow("c549") << QU8("Ī") << QByteArray::fromHex("c549");
  QTest::newRow("c54f") << QU8("Ō") << QByteArray::fromHex("c54f");
  QTest::newRow("c555") << QU8("Ū") << QByteArray::fromHex("c555");
  QTest::newRow("c561") << QU8("ā") << QByteArray::fromHex("c561");
  QTest::newRow("c565") << QU8("ē") << QByteArray::fromHex("c565");
  QTest::newRow("c569") << QU8("ī") << QByteArray::fromHex("c569");
  QTest::newRow("c56f") << QU8("ō") << QByteArray::fromHex("c56f");
  QTest::newRow("c575") << QU8("ū") << QByteArray::fromHex("c575");
  QTest::newRow("c641") << QU8("Ă") << QByteArray::fromHex("c641");
  QTest::newRow("c647") << QU8("Ğ") << QByteArray::fromHex("c647");
  QTest::newRow("c655") << QU8("Ŭ") << QByteArray::fromHex("c655");
  QTest::newRow("c661") << QU8("ă") << QByteArray::fromHex("c661");
  QTest::newRow("c667") << QU8("ğ") << QByteArray::fromHex("c667");
  QTest::newRow("c675") << QU8("ŭ") << QByteArray::fromHex("c675");
  QTest::newRow("c743") << QU8("Ċ") << QByteArray::fromHex("c743");
  QTest::newRow("c745") << QU8("Ė") << QByteArray::fromHex("c745");
  QTest::newRow("c747") << QU8("Ġ") << QByteArray::fromHex("c747");
  QTest::newRow("c749") << QU8("İ") << QByteArray::fromHex("c749");
  QTest::newRow("c75a") << QU8("Ż") << QByteArray::fromHex("c75a");
  QTest::newRow("c763") << QU8("ċ") << QByteArray::fromHex("c763");
  QTest::newRow("c765") << QU8("ė") << QByteArray::fromHex("c765");
  QTest::newRow("c767") << QU8("ġ") << QByteArray::fromHex("c767");
  QTest::newRow("c77a") << QU8("ż") << QByteArray::fromHex("c77a");
  QTest::newRow("c841") << QU8("Ä") << QByteArray::fromHex("c841");
  QTest::newRow("c845") << QU8("Ë") << QByteArray::fromHex("c845");
  QTest::newRow("c849") << QU8("Ï") << QByteArray::fromHex("c849");
  QTest::newRow("c84f") << QU8("Ö") << QByteArray::fromHex("c84f");
  QTest::newRow("c855") << QU8("Ü") << QByteArray::fromHex("c855");
  QTest::newRow("c859") << QU8("Ÿ") << QByteArray::fromHex("c859");
  QTest::newRow("c861") << QU8("ä") << QByteArray::fromHex("c861");
  QTest::newRow("c865") << QU8("ë") << QByteArray::fromHex("c865");
  QTest::newRow("c869") << QU8("ï") << QByteArray::fromHex("c869");
  QTest::newRow("c86f") << QU8("ö") << QByteArray::fromHex("c86f");
  QTest::newRow("c875") << QU8("ü") << QByteArray::fromHex("c875");
  QTest::newRow("c879") << QU8("ÿ") << QByteArray::fromHex("c879");
  QTest::newRow("ca41") << QU8("Å") << QByteArray::fromHex("ca41");
  QTest::newRow("ca55") << QU8("Ů") << QByteArray::fromHex("ca55");
  QTest::newRow("ca61") << QU8("å") << QByteArray::fromHex("ca61");
  QTest::newRow("ca75") << QU8("ů") << QByteArray::fromHex("ca75");
  QTest::newRow("cb43") << QU8("Ç") << QByteArray::fromHex("cb43");
  QTest::newRow("cb47") << QU8("Ģ") << QByteArray::fromHex("cb47");
  QTest::newRow("cb4b") << QU8("Ķ") << QByteArray::fromHex("cb4b");
  QTest::newRow("cb4c") << QU8("Ļ") << QByteArray::fromHex("cb4c");
  QTest::newRow("cb4e") << QU8("Ņ") << QByteArray::fromHex("cb4e");
  QTest::newRow("cb52") << QU8("Ŗ") << QByteArray::fromHex("cb52");
  QTest::newRow("cb53") << QU8("Ş") << QByteArray::fromHex("cb53");
  QTest::newRow("cb54") << QU8("Ţ") << QByteArray::fromHex("cb54");
  QTest::newRow("cb63") << QU8("ç") << QByteArray::fromHex("cb63");
  QTest::newRow("cb67") << QU8("ģ") << QByteArray::fromHex("cb67");
  QTest::newRow("cb6b") << QU8("ķ") << QByteArray::fromHex("cb6b");
  QTest::newRow("cb6c") << QU8("ļ") << QByteArray::fromHex("cb6c");
  QTest::newRow("cb6e") << QU8("ņ") << QByteArray::fromHex("cb6e");
  QTest::newRow("cb72") << QU8("ŗ") << QByteArray::fromHex("cb72");
  QTest::newRow("cb73") << QU8("ş") << QByteArray::fromHex("cb73");
  QTest::newRow("cb74") << QU8("ţ") << QByteArray::fromHex("cb74");
  QTest::newRow("cd4f") << QU8("Ő") << QByteArray::fromHex("cd4f");
  QTest::newRow("cd55") << QU8("Ű") << QByteArray::fromHex("cd55");
  QTest::newRow("cd6f") << QU8("ő") << QByteArray::fromHex("cd6f");
  QTest::newRow("cd75") << QU8("ű") << QByteArray::fromHex("cd75");
  QTest::newRow("ce41") << QU8("Ą") << QByteArray::fromHex("ce41");
  QTest::newRow("ce45") << QU8("Ę") << QByteArray::fromHex("ce45");
  QTest::newRow("ce49") << QU8("Į") << QByteArray::fromHex("ce49");
  QTest::newRow("ce55") << QU8("Ų") << QByteArray::fromHex("ce55");
  QTest::newRow("ce61") << QU8("ą") << QByteArray::fromHex("ce61");
  QTest::newRow("ce65") << QU8("ę") << QByteArray::fromHex("ce65");
  QTest::newRow("ce69") << QU8("į") << QByteArray::fromHex("ce69");
  QTest::newRow("ce75") << QU8("ų") << QByteArray::fromHex("ce75");
  QTest::newRow("cf43") << QU8("Č") << QByteArray::fromHex("cf43");
  QTest::newRow("cf44") << QU8("Ď") << QByteArray::fromHex("cf44");
  QTest::newRow("cf45") << QU8("Ě") << QByteArray::fromHex("cf45");
  QTest::newRow("cf4c") << QU8("Ľ") << QByteArray::fromHex("cf4c");
  QTest::newRow("cf4e") << QU8("Ň") << QByteArray::fromHex("cf4e");
  QTest::newRow("cf52") << QU8("Ř") << QByteArray::fromHex("cf52");
  QTest::newRow("cf53") << QU8("Š") << QByteArray::fromHex("cf53");
  QTest::newRow("cf54") << QU8("Ť") << QByteArray::fromHex("cf54");
  QTest::newRow("cf5a") << QU8("Ž") << QByteArray::fromHex("cf5a");
  QTest::newRow("cf63") << QU8("č") << QByteArray::fromHex("cf63");
  QTest::newRow("cf64") << QU8("ď") << QByteArray::fromHex("cf64");
  QTest::newRow("cf65") << QU8("ě") << QByteArray::fromHex("cf65");
  QTest::newRow("cf6c") << QU8("ľ") << QByteArray::fromHex("cf6c");
  QTest::newRow("cf6e") << QU8("ň") << QByteArray::fromHex("cf6e");
  QTest::newRow("cf72") << QU8("ř") << QByteArray::fromHex("cf72");
  QTest::newRow("cf73") << QU8("š") << QByteArray::fromHex("cf73");
  QTest::newRow("cf74") << QU8("ť") << QByteArray::fromHex("cf74");
  QTest::newRow("cf7a") << QU8("ž") << QByteArray::fromHex("cf7a");
  QTest::newRow("a8") << QU8("¤") << QByteArray::fromHex("a8");
  QTest::newRow("d2") << QU8("®") << QByteArray::fromHex("d2");
  QTest::newRow("d3") << QU8("©") << QByteArray::fromHex("d3");
  QTest::newRow("d6") << QU8("¬") << QByteArray::fromHex("d6");
  QTest::newRow("d7") << QU8("¦") << QByteArray::fromHex("d7");
  QTest::newRow("e3") << QU8("ª") << QByteArray::fromHex("e3");
  QTest::newRow("ff") << QU8("\u00ad") << QByteArray::fromHex("ff");
}
