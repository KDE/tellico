/***************************************************************************
    Copyright (C) 2009-2020 Robby Stephenson <robby@periapsis.org>
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

#include "formattest.h"

#include "../fieldformat.h"
#include "../config/tellico_config.h"

#include <KLocalizedString>

#include <QTest>

QTEST_GUILESS_MAIN( FormatTest )

void FormatTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::Config::setArticlesString(QStringLiteral("the,l'"));
  Tellico::Config::setNoCapitalizationString(QStringLiteral("the,of,et,de"));
}

void FormatTest::testCapitalization() {
  QFETCH(QString, string);
  QFETCH(QString, capitalized);

  QCOMPARE(Tellico::FieldFormat::capitalize(string), capitalized);
}

void FormatTest::testCapitalization_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("capitalized");

  QTest::newRow("test1") << "the title" << "The Title";
  // right now, 'the' is in the no capitalization string
  QTest::newRow("test2") << "title, the" << "Title, the";
  QTest::newRow("test3") << "the return of the king" << "The Return of the King";
  QTest::newRow("test4") << QStringLiteral("école nationale supérieure de l'aéronautique et de l'espace")
                         << QStringLiteral("École Nationale Supérieure de l'Aéronautique et de l'Espace");
  QTest::newRow("test5") << "Appartement, L'" << "Appartement, L'";
}

void FormatTest::testTitle() {
  QFETCH(QString, string);
  QFETCH(QString, formatted);
  QFETCH(bool, capitalize);
  QFETCH(bool, format);

  Tellico::FieldFormat::Options options;
  if(capitalize) {
    options |= Tellico::FieldFormat::FormatCapitalize;
  }
  if(format) {
    options |= Tellico::FieldFormat::FormatAuto;
  }

  QCOMPARE(Tellico::FieldFormat::title(string, options), formatted);
}

void FormatTest::testTitle_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("formatted");
  QTest::addColumn<bool>("capitalize");
  QTest::addColumn<bool>("format");

  QTest::newRow("test1") << "the title" << "the title" << false << false;
  QTest::newRow("test2") << "the title" << "The Title" << true << false;
  // right now, 'the' is in the no capitalization string
  QTest::newRow("test3") << "title, the" << "Title, the" << true << false;
  QTest::newRow("test4") << "the Title" << "the Title" << false << false;
  QTest::newRow("test5") << "the Title" << "The Title" << true << false;
  QTest::newRow("test6") << "the title" << "title, the" << false << true;
  QTest::newRow("test7") << "the title" << "Title, The" << true << true;
  // right now, 'the' is in the no capitalization string
  QTest::newRow("test8") << "title,the" << "Title, the" << true << true;
  QTest::newRow("test9") << "the return of the king" << "Return of the King, The" << true << true;
}

void FormatTest::testName() {
  QFETCH(QString, string);
  QFETCH(QString, formatted);
  QFETCH(bool, capitalize);
  QFETCH(bool, format);

  Tellico::FieldFormat::Options options;
  if(capitalize) {
    options |= Tellico::FieldFormat::FormatCapitalize;
  }
  if(format) {
    options |= Tellico::FieldFormat::FormatAuto;
  }

  QCOMPARE(Tellico::FieldFormat::name(string, options), formatted);
}

void FormatTest::testName_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("formatted");
  QTest::addColumn<bool>("capitalize");
  QTest::addColumn<bool>("format");

  QTest::newRow("test1") << "name" << "Name" << true << false;
  QTest::newRow("test2") << "first name" << "name, first" << false << true;
  QTest::newRow("test3") << "first Name" << "Name, First" << true << true;
  QTest::newRow("test4") << "tom swift, jr." << "Swift, Jr., Tom" << true << true;
  QTest::newRow("test4-capitalize") << "tom swift, Jr." << "Swift, Jr., Tom" << true << true;
  QTest::newRow("test5") << "swift, jr., tom" << "Swift, Jr., Tom" << true << false;
  QTest::newRow("test6") << "tom de swift, jr." << "de Swift, Jr., Tom" << true << true;
  QTest::newRow("test6-capitalize") << "tom de swift, Jr." << "de Swift, Jr., Tom" << true << true;
  QTest::newRow("test7") << "dr.  tom de swift ,  jr." << "Dr. Tom de Swift, Jr." << true << false;
  QTest::newRow("test8") << "dr.  tom de swift ,  jr." << "de Swift, Jr., Dr. Tom" << true << true;
  QTest::newRow("test9") << "hannibal lector smith" << "Smith, Hannibal Lector" << true << true;
  QTest::newRow("test10") << "hannibal lector-smith" << "Lector-Smith, Hannibal" << true << true;
  QTest::newRow("test11") << "lector smith, hannibal" << "Lector Smith, Hannibal" << true << true;
  QTest::newRow("test12") << "john  van der graf" << "van der Graf, John" << true << true;
  QTest::newRow("test13") << "john  Van Der graf" << "Van Der Graf, John" << true << true;
  QTest::newRow("single comma") << "," << "," << false << true;
}

void FormatTest::testDate() {
  QFETCH(QString, string);
  QFETCH(QString, expected);

  QString formatted = Tellico::FieldFormat::format(string, Tellico::FieldFormat::FormatDate, Tellico::FieldFormat::AsIsFormat);
  QCOMPARE(formatted, expected);
}

void FormatTest::testDate_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("expected");

  QTest::newRow("text") << "text" << "text"; // just returns input
  QTest::newRow("date1") << "2025-01-01" << "2025-01-01";
  QTest::newRow("date2") << "2025-1-1" << "2025-01-01";
  // default to first month
  QTest::newRow("date3") << "2025--1" << "2025-01-01";
  // default to first day
  QTest::newRow("date3") << "2025-1" << "2025-01-01";
  // default to current year
  const int y = QDate::currentDate().year();
  QTest::newRow("date4") << "--1" << QString::number(y) + "-01-01";
}

void FormatTest::testSplit() {
  QStringList list = QStringList() << QStringLiteral("one") << QStringLiteral("two") << QStringLiteral("three");
  QCOMPARE(Tellico::FieldFormat::splitValue(list.join(Tellico::FieldFormat::delimiterString())), list);
  QCOMPARE(Tellico::FieldFormat::splitRow(list.join(Tellico::FieldFormat::columnDelimiterString())), list);
  QCOMPARE(Tellico::FieldFormat::splitTable(list.join(Tellico::FieldFormat::rowDelimiterString())), list);
}

void FormatTest::testStripArticles() {
  QFETCH(QString, articles);
  QFETCH(QString, string);
  QFETCH(QString, stripped);

  Tellico::Config::setArticlesString(articles);
  Tellico::FieldFormat::stripArticles(string);
  QCOMPARE(string, stripped);
}

void FormatTest::testStripArticles_data() {
  QTest::addColumn<QString>("articles");
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("stripped");

  QTest::newRow("test1") << "the,l'" << "name" << "name";
  QTest::newRow("test2") << "the,l'" << "the name" << "name";
  QTest::newRow("test3") << "the,l'" << "name, the" << "name";
  QTest::newRow("test4") << "the,l'" << "thename" << "thename";
  QTest::newRow("test5") << "the,l'" << "l'name" << "name";
  QTest::newRow("test6") << "the,l'" << "lname" << "lname";
  QTest::newRow("test7") << "the,l'" << "al'name" << "al'name";
  QTest::newRow("test8") << "the" << "l'name" << "l'name";
  QTest::newRow("test9") << "l'" << "the name" << "the name";
  QTest::newRow("test10") << "l'" << "l'name," << "name";
}
