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

#include "isbntest.h"

#include "../utils/isbnvalidator.h"

#include <QTest>

QTEST_APPLESS_MAIN( IsbnTest )

Q_DECLARE_METATYPE(QValidator::State)

#define QL1(x) QString::fromLatin1(x)

void IsbnTest::initTestCase() {
  qRegisterMetaType<QValidator::State>();
}

void IsbnTest::testFixup() {
  QFETCH(QString, string);
  QFETCH(QString, expectedIsbn);

  Tellico::ISBNValidator val(0);
  QString qs = string;
  val.fixup(qs);
  QCOMPARE(qs, expectedIsbn);
}

void IsbnTest::testFixup_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("expectedIsbn");

  // garbage
  QTest::newRow("My name is robby") << QL1("My name is robby") << QString();
  QTest::newRow("http://www.abclinuxu.cz/clanky/show/63080") << QL1("http://www.abclinuxu.cz/clanky/show/63080") << QL1("6-3080");

  // initial checks
  QTest::newRow("0-446-60098-9") << QL1("0-446-60098-9") << QL1("0-446-60098-9");
  // check sum value
  QTest::newRow("0-446-60098") << QL1("0-446-60098") << QL1("0-446-60098-9");

  // check EAN-13
  QTest::newRow("9780940016750") << QL1("9780940016750") << QL1("978-0-940016-75-0");
  QTest::newRow("978-0940016750") << QL1("978-0940016750") << QL1("978-0-940016-75-0");
  QTest::newRow("978-0-940016-75-0") << QL1("978-0-940016-75-0") << QL1("978-0-940016-75-0");
  QTest::newRow("978286274486") << QL1("978286274486") << QL1("978-2-86274-486-5");
  QTest::newRow("9788186119130") << QL1("9788186119130") << QL1("978-81-86-11913-6");
  QTest::newRow("9788186119137") << QL1("9788186119137") << QL1("978-81-86-11913-6");
  QTest::newRow("97881-8611-9-13-0") << QL1("97881-8611-9-13-0") << QL1("978-81-86-11913-6");
  QTest::newRow("97881-8611-9-13-7") << QL1("97881-8611-9-13-7") << QL1("978-81-86-11913-6");

  // don't add checksum for EAN that start with 978 or 979 and are less than 13 in length
  QTest::newRow("978059600") << QL1("978059600") << QL1("978-059600");
  QTest::newRow("978-0596000") << QL1("978-0596000") << QL1("978-059600-0");

  // normal english-language hyphenation
  QTest::newRow("0") << QL1("0") << QL1("0");
  QTest::newRow("05") << QL1("05") << QL1("0-5");
  QTest::newRow("059") << QL1("059") << QL1("0-59");
  QTest::newRow("0596") << QL1("0596") << QL1("0-596");
  QTest::newRow("05960") << QL1("05960") << QL1("0-596-0");
  QTest::newRow("059600") << QL1("059600") << QL1("0-596-00");
  QTest::newRow("0596000") << QL1("0596000") << QL1("0-596-000");
  QTest::newRow("05960005") << QL1("05960005") << QL1("0-596-0005");
  // checksum gets added
  QTest::newRow("059600053") << QL1("059600053") << QL1("0-596-00053-7");
  QTest::newRow("0-596-00053") << QL1("0-596-00053") << QL1("0-596-00053-7");
  QTest::newRow("044660098") << QL1("044660098") << QL1("0-446-60098-9");
  QTest::newRow("0446600989") << QL1("0446600989") << QL1("0-446-60098-9");

  // check french hyphenation
  QTest::newRow("2862744867") << QL1("2862744867") << QL1("2-86274-486-7");

  // check german hyphenation
  QTest::newRow("3423071516") << QL1("3423071516") << QL1("3-423-07151-6");

  // check polish hyphenation
  QTest::newRow("978-83-7436-170-5") << QL1("9788374361705") << QL1("978-83-7436-170-5");

  // check keeping middle hyphens
  QTest::newRow("6-18611913-0") << QL1("6-18611913-0") << QL1("6-18611913-0");
  QTest::newRow("6-186119-13-0") << QL1("6-186119-13-0") << QL1("6-186119-13-0");
  QTest::newRow("6-18611-9-13-0") << QL1("6-18611-9-13-0") << QL1("6-18611-913-0");
}

void IsbnTest::testIsbn10() {
  QFETCH(QString, string);
  QFETCH(QString, expectedIsbn);

  QCOMPARE(Tellico::ISBNValidator::isbn10(string), expectedIsbn);
}

void IsbnTest::testIsbn10_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("expectedIsbn");

  QTest::newRow("0-06-087298-5") << QL1("0-06-087298-5") << QL1("0-06-087298-5");
  QTest::newRow("978-0-06-087298-4") << QL1("978-0-06-087298-4") << QL1("0-06-087298-5");
}

void IsbnTest::testIsbn13() {
  QFETCH(QString, string);
  QFETCH(QString, expectedIsbn);

  QCOMPARE(Tellico::ISBNValidator::isbn13(string), expectedIsbn);
}

void IsbnTest::testIsbn13_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("expectedIsbn");

  QTest::newRow("0-06-087298-5") << QL1("0-06-087298-5") << QL1("978-0-06-087298-4");
  QTest::newRow("9780-06-087298-4") << QL1("9780-06-087298-4") << QL1("978-0-06-087298-4");
}

void IsbnTest::testComparison() {
  QFETCH(QString, value1);
  QFETCH(QString, value2);
  QFETCH(bool, equal);

  Tellico::ISBNComparison comp;
  QCOMPARE(comp(value1, value2), equal);
}

void IsbnTest::testComparison_data() {
  QTest::addColumn<QString>("value1");
  QTest::addColumn<QString>("value2");
  QTest::addColumn<bool>("equal");

  QTest::newRow("0446600989, 0-446-60098-9") << QL1("0446600989") << QL1("0-446-60098-9") << true;
  QTest::newRow("0940016753, 9780940016750") << QL1("0940016753") << QL1("9780940016750") << true;
  QTest::newRow("9780940016750, 0940016753") << QL1("9780940016750") << QL1("0940016753") << true;
  QTest::newRow("9780940016750, 978-0-940016-75-0") << QL1("9780940016750") << QL1("978-0-940016-75-0") << true;
  QTest::newRow("3-351-005296, 3351005296") << QL1("3-351-005296") <<  QL1("3351005296") << true;
  QTest::newRow("3-351-00529-6, 3351005296") << QL1("3-351-00529-6") <<  QL1("3351005296") << true;
}

void IsbnTest::testListDifference() {
  QFETCH(QStringList, list1);
  QFETCH(QStringList, list2);
  QFETCH(QStringList, result);

  QCOMPARE(Tellico::ISBNValidator::listDifference(list1, list2), result);
}

void IsbnTest::testListDifference_data() {
  QTest::addColumn<QStringList>("list1");
  QTest::addColumn<QStringList>("list2");
  QTest::addColumn<QStringList>("result");

  QStringList list1;
  list1 << QLatin1String("0940016753") << QLatin1String("9780940016750");
  QStringList list2;

  // comparing to empty list should return the first list
  QTest::newRow("list1") << list1 << list2 << list1;

  // comparing to a value that matches everything in the list should return empty list
  list2 << QLatin1String("0-940016-75-0");
  QTest::newRow("list2") << list1 << list2 << QStringList();
}

void IsbnTest::testState() {
  QFETCH(QValidator::State, expectedState);
  QFETCH(QString, value);
  QFETCH(bool, changedValue);

  int pos = value.length() - 1;
  const QString original = value;

  Tellico::ISBNValidator val(0);
  QValidator::State state = val.validate(value, pos);
  if(!changedValue) {
    QCOMPARE(value, original);
  }
  QCOMPARE(state, expectedState);
}

void IsbnTest::testState_data() {
  QTest::addColumn<QValidator::State>("expectedState");
  QTest::addColumn<QString>("value");
  QTest::addColumn<bool>("changedValue");

  QTest::newRow("0") << QValidator::Intermediate << QL1("0") << false;
  QTest::newRow("0-") << QValidator::Intermediate << QL1("0-") << false;
  QTest::newRow("0-3") << QValidator::Intermediate << QL1("0-3") << false;
  QTest::newRow("0-32") << QValidator::Intermediate << QL1("0-32") << false;
  QTest::newRow("0-321") << QValidator::Intermediate << QL1("0-321") << false;
  QTest::newRow("0-321-") << QValidator::Intermediate << QL1("0-321-") << false;
  QTest::newRow("0-321-1") << QValidator::Intermediate << QL1("0-321-1") << false;
  QTest::newRow("0-321-11") << QValidator::Intermediate << QL1("0-321-11") << false;
  QTest::newRow("0-321-113") << QValidator::Intermediate << QL1("0-321-113") << false;
  QTest::newRow("0-321-1135") << QValidator::Intermediate << QL1("0-321-1135") << false;
  // checksum is added
  QTest::newRow("0-321-11358") << QValidator::Acceptable << QL1("0-321-11358") << true;
  QTest::newRow("0-321-11358-") << QValidator::Acceptable << QL1("0-321-11358-") << true;
  QTest::newRow("0-321-11358-6") << QValidator::Acceptable << QL1("0-321-11358-6") << false;

  QTest::newRow("0") << QValidator::Intermediate << QL1("0") << true;
  QTest::newRow("03") << QValidator::Intermediate << QL1("03") << true;
  QTest::newRow("032") << QValidator::Intermediate << QL1("032") << true;
  QTest::newRow("0321") << QValidator::Intermediate << QL1("0321") << true;
  QTest::newRow("03211") << QValidator::Intermediate << QL1("03211") << true;
  QTest::newRow("032111") << QValidator::Intermediate << QL1("032111") << true;
  QTest::newRow("0321113") << QValidator::Intermediate << QL1("0321113") << true;
  QTest::newRow("03211135") << QValidator::Intermediate << QL1("03211135") << true;
  // checksum is added
  QTest::newRow("032111358") << QValidator::Acceptable << QL1("032111358") << true;
  QTest::newRow("0321113586") << QValidator::Acceptable << QL1("0321113586") << true;

  // considered 10-digit ISBNs
  QTest::newRow("9") << QValidator::Intermediate << QL1("9") << false;
  QTest::newRow("97") << QValidator::Intermediate << QL1("97") << false;
  QTest::newRow("978") << QValidator::Intermediate << QL1("978") << false;
  QTest::newRow("978-") << QValidator::Intermediate << QL1("978-") << false;
  QTest::newRow("978-0") << QValidator::Intermediate << QL1("978-0") << false;
  QTest::newRow("978-0-") << QValidator::Intermediate << QL1("978-0-") << false;
  QTest::newRow("978-0-4") << QValidator::Intermediate << QL1("978-0-4") << false;
  QTest::newRow("978-0-47") << QValidator::Intermediate << QL1("978-0-47") << false;
  QTest::newRow("978-0-470") << QValidator::Intermediate << QL1("978-0-470") << false;
  QTest::newRow("978-0-470-") << QValidator::Intermediate << QL1("978-0-470-") << false;
  QTest::newRow("978-0-4701") << QValidator::Intermediate << QL1("978-0-4701") << false;
  QTest::newRow("978-0-470-1") << QValidator::Intermediate << QL1("978-0-470-1") << true;
  QTest::newRow("978-0-47014") << QValidator::Intermediate << QL1("978-0-47014") << false;
  QTest::newRow("978-0-470-14") << QValidator::Intermediate << QL1("978-0-470-14") << true;
  QTest::newRow("978-0-470-147") << QValidator::Intermediate << QL1("978-0-470-147") << true;
  QTest::newRow("978-0-470147") << QValidator::Intermediate << QL1("978-0-470147") << true;
  QTest::newRow("978-0-47014-7") << QValidator::Intermediate << QL1("978-0-47014-7") << false;
  // now considered 13-digit ISBN
  QTest::newRow("978-0-470-1476") << QValidator::Intermediate << QL1("978-0-470-1476") << false;
  // checksum is added
  QTest::newRow("978-0-470-14762") << QValidator::Acceptable << QL1("978-0-470-14762") << true;
  QTest::newRow("978-0-470-14762-") << QValidator::Acceptable << QL1("978-0-470-14762-") << true;
  QTest::newRow("978-0-470-14762-7") << QValidator::Acceptable << QL1("978-0-470-14762-7") << false;
}
