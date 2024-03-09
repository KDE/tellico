/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#undef QT_NO_CAST_FROM_ASCII

#include "entitytest.h"

#include "../utils/string_utils.h"
#include "../utils/mapvalue.h"

#include <KLocalizedString>

#include <QTest>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>

QTEST_APPLESS_MAIN( EntityTest )

#define QSL(x) QStringLiteral(x)

void EntityTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
}

void EntityTest::testEntities() {
  QFETCH(QByteArray, data);
  QFETCH(QString, expectedString);

  QCOMPARE(Tellico::decodeHTML(data), expectedString);
}

void EntityTest::testEntities_data() {
  QTest::addColumn<QByteArray>("data");
  QTest::addColumn<QString>("expectedString");

  QTest::newRow("robby") << QByteArray("robby") << QSL("robby");
  QTest::newRow("&fake;") << QByteArray("&fake;") << QSL("&fake;");
  QTest::newRow("&#48;") << QByteArray("&#48;") << QSL("0");
  QTest::newRow("robby&#48;robb") << QByteArray("robby&#48;robby") << QSL("robby0robby");
}

void EntityTest::testAccents() {
  QFETCH(QString, inputString);
  QFETCH(QString, expectedString);

  QCOMPARE(Tellico::removeAccents(inputString), expectedString);
}

void EntityTest::testAccents_data() {
  QTest::addColumn<QString>("inputString");
  QTest::addColumn<QString>("expectedString");

  QTest::newRow("robby") << QSL("robby") << QSL("robby");
  QTest::newRow("jose") << QSL("José Guzmán") << QSL("Jose Guzman");
  QTest::newRow("inarritu") << QSL("Alejandro González Iñárritu") << QSL("Alejandro Gonzalez Inarritu");
  QTest::newRow("harakiri") << QSL("'Shitsurakuen': jôbafuku onna harakiri") << QSL("'Shitsurakuen': jobafuku onna harakiri");
  QTest::newRow("svet") << QSL("Tmavomodrý Svět") << QSL("Tmavomodry Svet");
  QTest::newRow("russian") << QSL("Возвращение Супермена") << QSL("Возвращение Супермена");
  QTest::newRow("chinese") << QSL("湖南科学技术出版社") << QSL("湖南科学技术出版社");
}

void EntityTest::testI18nReplace() {
  QFETCH(QString, inputString);
  QFETCH(QString, expectedString);

  QCOMPARE(Tellico::i18nReplace(inputString), expectedString);
}

void EntityTest::testI18nReplace_data() {
  QTest::addColumn<QString>("inputString");
  QTest::addColumn<QString>("expectedString");

  QTest::newRow("robby") << QSL("robby") << QSL("robby");
  QTest::newRow("basic1") << QSL("<i18n>robby</i18n>") << QSL("robby");
  QTest::newRow("basic2") << QSL("<i18n>robby davy</i18n>") << QSL("robby davy");
  QTest::newRow("basic3") << QSL("\n   <i18n>robby</i18n>  \n davy\n") << QSL("robby davy\n");
  // KDE bug 254863
  QTest::newRow("bug254863") << QSL("<i18n>Cer&ca</i18n>") << QSL("Cer&amp;ca");
  QTest::newRow("multiple") << QSL("<i18n>robby</i18n> davy <i18n>jason</i18n>") << QSL("robby davy jason");
  QTest::newRow("bracket") << QSL("<i18n>robby <robby></i18n>") << QSL("robby &lt;robby&gt;");
}

void EntityTest::testMinutes() {
  QFETCH(int, seconds);
  QFETCH(QString, minutesString);

  QCOMPARE(Tellico::minutes(seconds), minutesString);
}

void EntityTest::testMinutes_data() {
  QTest::addColumn<int>("seconds");
  QTest::addColumn<QString>("minutesString");

  QTest::newRow("1")   << 1   << QSL("0:01");
  QTest::newRow("60")  << 60  << QSL("1:00");
  QTest::newRow("600") << 600 << QSL("10:00");
  QTest::newRow("0")   << 0   << QSL("0:00");
  QTest::newRow("120") << 120 << QSL("2:00");
}

void EntityTest::testObfuscate() {
  QString s(QStringLiteral("1q!Q+=% f"));
  QByteArray b = Tellico::obfuscate(s);
//  qDebug() << s << b;
  QCOMPARE(s, Tellico::reverseObfuscate(b));
}

void EntityTest::testControlCodes() {
  QFETCH(QString, string);
  QFETCH(QString, result);

  QCOMPARE(Tellico::removeControlCodes(string), result);
}

void EntityTest::testControlCodes_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("result");

  QTest::newRow("basic test") << QSL("basic test") << QSL("basic test");
  QTest::newRow("basic test1") << (QSL("basic test") + QChar(1)) << QSL("basic test");
  QTest::newRow("nl") << QSL("new\nline") << QSL("new\nline");
  QTest::newRow("cr") << QSL("new\rline") << QSL("new\rline");
  QTest::newRow("tab") << QSL("new\ttab") << QSL("new\ttab");
  QTest::newRow("hex5") << QSL("hex5\x5") << QSL("hex5");
  QTest::newRow("hexD") << QSL("hexD\xD") << QSL("hexD\xD");
}

void EntityTest::testBug254863() {
  QString input(QLatin1String("\\n<div>\\n\\n<i18n>check&replace</i18n></div>\\n"));
  auto output = Tellico::i18nReplace(input);
  QVERIFY(!output.contains(QLatin1String("i18n>")));
  QVERIFY(!output.contains(QLatin1String("&replace")));
  static const QRegularExpression whiteSpaceRx(QLatin1String("\\s$"));
  QVERIFY(!output.contains(whiteSpaceRx));
}

void EntityTest::testMapValue() {
  auto data(QByteArrayLiteral(R"(
{
 "num": 10,
 "item":"value",
 "items": [
    {"title":"title"},
    {"title":"title2"}
 ],
 "chain": {
   "chain2": {
     "chain3": {
       "chain4":"value"
     }
   }
 },
 "list":["value1", "value2", "value3"],
 "list2":{"list":["value1", "value2", "value3"]}
}
)"));
  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVERIFY(!doc.isNull());
  auto map = doc.object().toVariantMap();
  QCOMPARE(Tellico::mapValue(map, "num"), QStringLiteral("10"));
  QCOMPARE(Tellico::mapValue(map, "item"), QStringLiteral("value"));
  QCOMPARE(Tellico::mapValue(map, "items", "title"), QStringLiteral("title; title2"));
  QCOMPARE(Tellico::mapValue(map, "chain", "chain2", "chain3", "chain4"), QStringLiteral("value"));
  QCOMPARE(Tellico::mapValue(map, "list"), QStringLiteral("value1; value2; value3"));
  QCOMPARE(Tellico::mapValue(map, "list2", "list"), QStringLiteral("value1; value2; value3"));
}
