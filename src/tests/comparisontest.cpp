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

#undef QT_NO_CAST_FROM_ASCII

#include "comparisontest.h"
#include "../models/stringcomparison.h"
#include "../models/fieldcomparison.h"
#include "../images/imagefactory.h"
#include "../config/tellico_config.h"

#include <KLocalizedString>

#include <QTest>

QTEST_GUILESS_MAIN( ComparisonTest )

void ComparisonTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::Config::setArticlesString(QStringLiteral("the,l'"));
  Tellico::ImageFactory::init();
}

void ComparisonTest::testNumber() {
  QFETCH(QString, string1);
  QFETCH(QString, string2);
  QFETCH(int, res);

  Tellico::NumberComparison comp;

  QCOMPARE(comp.compare(string1, string2), res);
}

void ComparisonTest::testNumber_data() {
  QTest::addColumn<QString>("string1");
  QTest::addColumn<QString>("string2");
  QTest::addColumn<int>("res");

  QTest::newRow("null") << QString() << QString() << 0;
  QTest::newRow("< 0") << QString() << QStringLiteral("0") << -1;
  QTest::newRow("> 0") << QStringLiteral("0") << QString() << 1;
  QTest::newRow("=1 1") << QStringLiteral("1") << QStringLiteral("1") << 0;
  QTest::newRow("< 1") << QStringLiteral("0") << QStringLiteral("1") << -1;
  QTest::newRow("> 1") << QStringLiteral("1") << QStringLiteral("0") << 1;
  QTest::newRow("> -1") << QStringLiteral("0") << QStringLiteral("-1") << 1;
  QTest::newRow("< -1") << QStringLiteral("-1") << QStringLiteral("0") << -1;
  QTest::newRow("> 10") << QStringLiteral("10") << QStringLiteral("5") << 5;
  QTest::newRow("< 10") << QStringLiteral("5") << QStringLiteral("10") << -5;
  QTest::newRow("multiple1") << QStringLiteral("1; 2") << QStringLiteral("2") << -1;
  QTest::newRow("multiple2") << QStringLiteral("3; 2") << QStringLiteral("2") << 1;
  QTest::newRow("multiple3") << QStringLiteral("2") << QStringLiteral("2; 3") << -1;
  QTest::newRow("multiple4") << QStringLiteral("1; 2") << QStringLiteral("1; 3") << -1;
  QTest::newRow("float1") << QStringLiteral("5.1") << QStringLiteral("6.9") << -2;
  QTest::newRow("float2") << QStringLiteral("5.1") << QStringLiteral("5.2") << -1;
  QTest::newRow("float3") << QStringLiteral("5.2") << QStringLiteral("5.1") << 1;
  QTest::newRow("float4") << QStringLiteral("5.1") << QStringLiteral("5.1") << 0;
}

void ComparisonTest::testLCC() {
  QFETCH(QString, string1);
  QFETCH(QString, string2);
  QFETCH(int, res);

  Tellico::LCCComparison comp;

  QCOMPARE(comp.compare(string1, string2), res);
}

void ComparisonTest::testLCC_data() {
  QTest::addColumn<QString>("string1");
  QTest::addColumn<QString>("string2");
  QTest::addColumn<int>("res");

  QTest::newRow("null") << QString() << QString() << 0;
  QTest::newRow("null1") << QString() << QStringLiteral("B") << -1;
  QTest::newRow("null2") << QStringLiteral("B") << QString() << 1;
  QTest::newRow("test1") << QStringLiteral("BX932 .C53 1993") << QStringLiteral("BX2230.3") << -1;
  QTest::newRow("test2") << QStringLiteral("BX932 .C53 1993") << QStringLiteral("BX2380 .R67 2002") << -1;
  QTest::newRow("test3") << QStringLiteral("AE25 E3 2002") << QStringLiteral("AE5 E333 2003") << 1;
}

void ComparisonTest::testDate() {
  QFETCH(QString, string1);
  QFETCH(QString, string2);
  QFETCH(int, res);

  Tellico::ISODateComparison comp;

  QCOMPARE(comp.compare(string1, string2), res);
}

void ComparisonTest::testDate_data() {
  QTest::addColumn<QString>("string1");
  QTest::addColumn<QString>("string2");
  QTest::addColumn<int>("res");

  QTest::newRow("null") << QString() << QString() << 0;
  QTest::newRow("null1") << QString() << QStringLiteral("2001") << -1;
  QTest::newRow("null2") << QStringLiteral("2001") << QString() << 1;
  QTest::newRow("test1") << QStringLiteral("2001-9-8") << QStringLiteral("2001-10-12") << -1;
  QTest::newRow("test2") << QStringLiteral("1998") << QStringLiteral("2020-01-01") << -1;
  QTest::newRow("test3") << QStringLiteral("2008--") << QStringLiteral("2008-1-1") << 0;
  QTest::newRow("test5") << QStringLiteral("2008-2-2") << QStringLiteral("2008-02-02") << 0;
  // non-integers get converted to current date
  QTest::newRow("words") << QStringLiteral("all-by-myself") << QStringLiteral("---") << 0;
  // no longer in 2020, so all dashes converts to current year, after 2020
  QTest::newRow("words") << QStringLiteral("2020--") << QStringLiteral("---") << -1;
}

void ComparisonTest::testTitle() {
  QFETCH(QString, string1);
  QFETCH(QString, string2);
  QFETCH(int, res);

  Tellico::TitleComparison comp;

  QCOMPARE(comp.compare(string1, string2), res);
}

void ComparisonTest::testTitle_data() {
  QTest::addColumn<QString>("string1");
  QTest::addColumn<QString>("string2");
  QTest::addColumn<int>("res");

  QTest::newRow("null") << QString() << QString() << 0;
  QTest::newRow("null1") << QString() << QStringLiteral("The One") << -1;
  QTest::newRow("null2") << QStringLiteral("The One") << QString() << 1;
  // not that they're equal, but that "The One" should be sorted under "One"
  QTest::newRow("test3") << QStringLiteral("The One") << QStringLiteral("One, The") << -1;
  QTest::newRow("test4") << QStringLiteral("The One") << QStringLiteral("one, the") << -1;
  QTest::newRow("test5") << QStringLiteral("l'One") << QStringLiteral("one, the") << -1;
  QTest::newRow("test6") << QStringLiteral("l'One") << QStringLiteral("the one") << 0;
  QTest::newRow("test7") << QStringLiteral("All One") << QStringLiteral("the one") << -1;
}

void ComparisonTest::testString() {
  QFETCH(QString, string1);
  QFETCH(QString, string2);
  QFETCH(int, res);

  Tellico::StringComparison comp;

  QCOMPARE(comp.compare(string1, string2), res);
}

void ComparisonTest::testString_data() {
  QTest::addColumn<QString>("string1");
  QTest::addColumn<QString>("string2");
  QTest::addColumn<int>("res");

  QTest::newRow("null") << QString() << QString() << 0;
  QTest::newRow("null1") << QString() << QStringLiteral("string") << -1;
  QTest::newRow("null2") << QStringLiteral("string") << QString() << 1;
  QTest::newRow("test1") << QStringLiteral("string1") << QStringLiteral("string1") << 0;
  QTest::newRow("test2") << QStringLiteral("string1") << QStringLiteral("string2") << -1;
}

void ComparisonTest::testBool() {
  QFETCH(QString, string1);
  QFETCH(QString, string2);
  QFETCH(int, res);

  Tellico::BoolComparison comp;

  QCOMPARE(comp.compare(string1, string2), res);
}

void ComparisonTest::testBool_data() {
  QTest::addColumn<QString>("string1");
  QTest::addColumn<QString>("string2");
  QTest::addColumn<int>("res");

  QTest::newRow("null") << QString() << QString() << 0;
  QTest::newRow("null1") << QString() << QStringLiteral("true") << -1;
  QTest::newRow("null2") << QStringLiteral("true") << QString() << 1;
  QTest::newRow("truetrue") << QStringLiteral("true") << QStringLiteral("true") << 0;
  QTest::newRow("truefalse") << QStringLiteral("true") << QStringLiteral("false") << 1;
  QTest::newRow("falsetrue") << QStringLiteral("false") << QStringLiteral("true") << -1;
}

void ComparisonTest::testChoiceField() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true)); // add default field

  QStringList allowed;
  allowed << QStringLiteral("choice2") << QStringLiteral("choice1");
  QVERIFY(allowed.size() > 1);
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("choice"), QStringLiteral("Choice"), allowed));
  coll->addField(field);

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(field, allowed.at(0));
  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  entry2->setField(field, allowed.at(1));

  Tellico::FieldComparison* comp = Tellico::FieldComparison::create(field);
  // even though the second allowed value would sort first, it comes second in the list
  QCOMPARE(comp->compare(entry1, entry2), -1);
}

void ComparisonTest::testImage() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true));
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("image"), QStringLiteral("Image"), Tellico::Data::Field::Image));
  coll->addField(field);

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));

  Tellico::FieldComparison* comp = Tellico::FieldComparison::create(field);
  QCOMPARE(comp->compare(entry1, entry2), 0);

  QUrl u1 = QUrl::fromLocalFile(QFINDTESTDATA("data/img1.jpg"));
  QString id1 = Tellico::ImageFactory::addImage(u1);
  QUrl u2 = QUrl::fromLocalFile(QFINDTESTDATA("../../icons/128-apps-tellico.png"));
  QString id2 = Tellico::ImageFactory::addImage(u2);

  entry1->setField(QLatin1String("image"), id1);
  QCOMPARE(comp->compare(entry1, entry2), 1);
  entry2->setField(QLatin1String("image"), id1);
  QCOMPARE(comp->compare(entry1, entry2), 0);
  entry2->setField(QLatin1String("image"), id2);
  QVERIFY(comp->compare(entry1, entry2) < 0);
}
