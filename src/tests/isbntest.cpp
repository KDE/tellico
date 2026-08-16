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

#define QL1(x) QStringLiteral(x)

void IsbnTest::initTestCase() {
  qRegisterMetaType<QValidator::State>();
}

void IsbnTest::testFixup() {
  QFETCH(QString, string);
  QFETCH(QString, expectedIsbn);

  Tellico::ISBNValidator val;
  QString qs = string;
  val.fixup(qs);
  QCOMPARE(qs, expectedIsbn);
}

void IsbnTest::testFixup_data() {
  QTest::addColumn<QString>("string");
  QTest::addColumn<QString>("expectedIsbn");

  // garbage
  QTest::newRow("My name is robby") << QL1("My name is robby") << QString();
  QTest::newRow("http://www.abclinuxu.cz/clanky/show/63080") << QL1("http://www.abclinuxu.cz/clanky/show/63080") << QL1("630-80");

  // initial checks
  QTest::newRow("0-446-60098-9") << QL1("0-446-60098-9") << QL1("0-446-60098-9");
  // check sum value
  QTest::newRow("0-446-60098") << QL1("0-446-60098") << QL1("0-446-60098-9");

  // check EAN-13
  QTest::newRow("9780940016750") << QL1("9780940016750") << QL1("978-0-940016-75-0");
  QTest::newRow("978-0940016750") << QL1("978-0940016750") << QL1("978-0-940016-75-0");
  QTest::newRow("978-0-940016-75-0") << QL1("978-0-940016-75-0") << QL1("978-0-940016-75-0");
  QTest::newRow("978286274486") << QL1("978286274486") << QL1("978-2-86274-486-5");
  QTest::newRow("9788186119130") << QL1("9788186119130") << QL1("978-81-86119-13-6");
  QTest::newRow("9788186119137") << QL1("9788186119137") << QL1("978-81-86119-13-6");
  QTest::newRow("97881-8611-9-13-0") << QL1("97881-8611-9-13-0") << QL1("978-81-86119-13-6");
  QTest::newRow("97881-8611-9-13-7") << QL1("97881-8611-9-13-7") << QL1("978-81-86119-13-6");

  // don't add checksum for EAN that start with 978 or 979 and are less than 13 in length
  QTest::newRow("978059600") << QL1("978059600") << QL1("978-059-600");
  QTest::newRow("978-0596000") << QL1("978-0596000") << QL1("978-059-600-0");

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

  // others
  QTest::newRow("978-99925-3-892-0") << QL1("9789992538920") << QL1("978-99925-3-892-0");
  QTest::newRow("978-99937-1-056-1") << QL1("9789993710561") << QL1("978-99937-1-056-1");
  QTest::newRow("979-8-88645-174-0") << QL1("9798886451740") << QL1("979-8-88645-174-0");
  QTest::newRow("979-10-90636-07-1") << QL1("9791090636071") << QL1("979-10-90636-07-1");
  // nigerian pre-2007
  QTest::newRow("978-123-456-7") << QL1("9781234567") << QL1("978-123-456-7");
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
  // can't convert, returns input
  QTest::newRow("979-10-90636-07-1") << QL1("9791090636071") << QL1("9791090636071");
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
  QTest::newRow("979-10-90636-07-1") << QL1("97910906360-71") << QL1("979-10-90636-07-1");
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
  list1 << QStringLiteral("0940016753") << QStringLiteral("9780940016750");
  QStringList list2;

  // comparing to empty list should return the first list
  QTest::newRow("list1") << list1 << list2 << list1;

  // comparing to a value that matches everything in the list should return empty list
  list2 << QStringLiteral("0-940016-75-0");
  QTest::newRow("list2") << list1 << list2 << QStringList();
}

void IsbnTest::testState() {
  QFETCH(QValidator::State, expectedState);
  QFETCH(QString, value);
  QFETCH(bool, changedValue);

  int pos = value.length();
  const QString original = value;

  Tellico::ISBNValidator val;
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

  QTest::newRow("f") << QValidator::Invalid << QL1("f") << false;
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
  // case where user likely deleted the last character, the check-sum. Instead of re-inserting it
  // delete the digit before as well
  QTest::newRow("0-321-11358-") << QValidator::Intermediate << QL1("0-321-11358-") << true;
  QTest::newRow("0-321-11358-6") << QValidator::Acceptable << QL1("0-321-11358-6") << false;

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

  // considered 10-digit ISBNs from nigeria
  QTest::newRow("9") << QValidator::Intermediate << QL1("9") << false;
  QTest::newRow("97") << QValidator::Intermediate << QL1("97") << false;
  QTest::newRow("978") << QValidator::Intermediate << QL1("978") << false;
  QTest::newRow("978-") << QValidator::Intermediate << QL1("978-") << false;
  QTest::newRow("978-0") << QValidator::Intermediate << QL1("978-0") << false;
  QTest::newRow("978-0-") << QValidator::Intermediate << QL1("978-0-") << false;
  QTest::newRow("978-04") << QValidator::Intermediate << QL1("978-04") << false;
  QTest::newRow("978-04-") << QValidator::Intermediate << QL1("978-04-") << false;
  QTest::newRow("978-047") << QValidator::Intermediate << QL1("978-047") << false;
  QTest::newRow("978-047-") << QValidator::Intermediate << QL1("978-047-") << false;
  QTest::newRow("978-047-0") << QValidator::Intermediate << QL1("978-047-0") << false;
  QTest::newRow("978-047-0-") << QValidator::Intermediate << QL1("978-047-0-") << false;
  QTest::newRow("978-047-01") << QValidator::Intermediate << QL1("978-047-01") << false;
  QTest::newRow("978-047-01-") << QValidator::Intermediate << QL1("978-047-01-") << true;
  QTest::newRow("978-047-014") << QValidator::Intermediate << QL1("978-047-014") << false;
  // case where we assume user deleted the check-sum
  QTest::newRow("978-047-014-") << QValidator::Intermediate << QL1("978-047-01") << false;
  // the 7 digit indicates it is now a possible isbn13, not a nigerian isbn10
  QTest::newRow("978-0-470-147") << QValidator::Intermediate << QL1("978-0-470-147") << true;
  QTest::newRow("978-0-470-147-") << QValidator::Intermediate << QL1("978-0-470-147-") << true;
  QTest::newRow("978-0-470-1476") << QValidator::Intermediate << QL1("978-0-470-1476") << false;
  // checksum is added
  QTest::newRow("978-0-470-14762") << QValidator::Acceptable << QL1("978-0-470-14762") << true;
  // user deleted the check-sum
  QTest::newRow("978-0-470-14762-") << QValidator::Intermediate << QL1("978-0-470-14762-") << true;
  QTest::newRow("978-0-470-14762-7") << QValidator::Acceptable << QL1("978-0-470-14762-7") << false;
  // invalid with a semi-colon and multiple values not allowed
  QTest::newRow("false multiple") << QValidator::Invalid << QL1("978-0-470-14762-7; 9") << false;
}

void IsbnTest::testMultiple() {
  QFETCH(QValidator::State, expectedState);
  QFETCH(QString, value);
  QFETCH(QString, newValue);

  int pos = value.length() - 1;

  Tellico::ISBNValidator val;
  val.setAllowMultiple(true);
  QValidator::State state = val.validate(value, pos);
  QCOMPARE(state, expectedState);
  QCOMPARE(value, newValue);
}

void IsbnTest::testMultiple_data() {
  QTest::addColumn<QValidator::State>("expectedState");
  QTest::addColumn<QString>("value");
  QTest::addColumn<QString>("newValue");

  QTest::newRow("multiple01") << QValidator::Acceptable << QL1("978-0-470-14762-7") << QL1("978-0-470-14762-7");
  QTest::newRow("multiple02") << QValidator::Intermediate << QL1("978-0-470-14762-7;") << QL1("978-0-470-14762-7; ");
  QTest::newRow("multiple03") << QValidator::Intermediate << QL1("978-0-470-14762-7; ") << QL1("978-0-470-14762-7; ");
  QTest::newRow("multiple04") << QValidator::Intermediate << QL1("978-0-470-14762-7;9") << QL1("978-0-470-14762-7; 9");
  QTest::newRow("multiple05") << QValidator::Intermediate << QL1("978-0-470-14762-7; 9") << QL1("978-0-470-14762-7; 9");
  QTest::newRow("multiple06") << QValidator::Intermediate << QL1("978-0-470-14762-7;  9") << QL1("978-0-470-14762-7; 9");
  QTest::newRow("multiple07") << QValidator::Intermediate << QL1("0321113586;03211135") << QL1("0-321-11358-6; 0-321-1135");
  QTest::newRow("multiple08") << QValidator::Acceptable << QL1("0321113586;032111358") << QL1("0-321-11358-6; 0-321-11358-6");
  QTest::newRow("multiple09") << QValidator::Acceptable << QL1("0321113586;0321113586") << QL1("0-321-11358-6; 0-321-11358-6");
  QTest::newRow("multiple10") << QValidator::Acceptable << QL1("0321113586;0321113586;0321113586") << QL1("0-321-11358-6; 0-321-11358-6; 0-321-11358-6");
  QTest::newRow("multiple11") << QValidator::Invalid << QL1("f;0321113586") << QL1("f; 0-321-11358-6");
}

void IsbnTest::testPos() {
  QFETCH(QString, value);
  QFETCH(int, pos);
  QFETCH(int, newPos);
  QFETCH(bool, multiple);

  Tellico::ISBNValidator val;
  val.setAllowMultiple(multiple);
  val.validate(value, pos);
  QCOMPARE(pos, newPos);
}

void IsbnTest::testPos_data() {
  QTest::addColumn<QString>("value");
  QTest::addColumn<int>("pos");
  QTest::addColumn<int>("newPos");
  QTest::addColumn<bool>("multiple");

  QTest::newRow("pos01") << QL1("978") << 2 << 2 << false;
  QTest::newRow("pos02") << QL1("9780") << 3 << 3 << false;
  QTest::newRow("pos03") << QL1("9780") << 4 << 5 << false; // 978-0
  QTest::newRow("pos04") << QL1("97804") << 3 << 3 << false;
  QTest::newRow("pos05") << QL1("97804") << 4 << 5 << false; // 978-04
  QTest::newRow("pos06") << QL1("97804") << 5 << 6 << false; // 978-04
  QTest::newRow("pos07") << QL1("97804") << 6 << 6 << false; // 978-04
  QTest::newRow("pos08") << QL1("978047") << 6 << 7 << false; // 978-047
  QTest::newRow("pos09") << QL1("978047") << 7 << 7 << false; // 978-047
  QTest::newRow("pos10") << QL1("0446600989") << 0 << 0 << false;
  QTest::newRow("pos11") << QL1("0446600989") << 1 << 1 << false; // 0-446-60098-9
  QTest::newRow("pos12") << QL1("0446600989") << 2 << 3 << false; // 0-446-60098-9
  QTest::newRow("pos13") << QL1("0446600989") << 3 << 4 << false; // 0-446-60098-9
  QTest::newRow("pos14") << QL1("0446600989") << 4 << 5 << false; // 0-446-60098-9
  QTest::newRow("pos15") << QL1("0446600989") << 5 << 7 << false; // 0-446-60098-9
  QTest::newRow("pos16") << QL1("0446600989") << 6 << 8 << false; // 0-446-60098-9
  QTest::newRow("pos17") << QL1("0446600989") << 7 << 9 << false; // 0-446-60098-9
  QTest::newRow("pos18") << QL1("0446600989") << 8 << 10 << false; // 0-446-60098-9
  QTest::newRow("pos19") << QL1("0446600989") << 9 << 11 << false; // 0-446-60098-9
  QTest::newRow("pos20") << QL1("0446600989") << 10 << 13 << false; // 0-446-60098-9
  QTest::newRow("pos21") << QL1("9780940016750") << 0 << 0 << false; // 978-0-940016-75-0
  QTest::newRow("pos22") << QL1("9780940016750") << 1 << 1 << false; // 978-0-940016-75-0
  QTest::newRow("pos23") << QL1("9780940016750") << 2 << 2 << false; // 978-0-940016-75-0
  QTest::newRow("pos24") << QL1("9780940016750") << 3 << 3 << false; // 978-0-940016-75-0
  QTest::newRow("pos25") << QL1("9780940016750") << 4 << 5 << false; // 978-0-940016-75-0
  QTest::newRow("pos26") << QL1("9780940016750") << 5 << 7 << false; // 978-0-940016-75-0
  QTest::newRow("pos27") << QL1("9780940016750") << 6 << 8 << false; // 978-0-940016-75-0
  QTest::newRow("pos28") << QL1("9780940016750") << 7 << 9 << false; // 978-0-940016-75-0
  QTest::newRow("pos29") << QL1("9780940016750") << 8 << 10 << false; // 978-0-940016-75-0
  QTest::newRow("pos30") << QL1("9780940016750") << 9 << 11 << false; // 978-0-940016-75-0
  QTest::newRow("pos31") << QL1("9780940016750") << 10 << 12 << false; // 978-0-940016-75-0
  QTest::newRow("pos32") << QL1("9780940016750") << 11 << 14 << false; // 978-0-940016-75-0
  QTest::newRow("pos33") << QL1("9780940016750") << 12 << 15 << false; // 978-0-940016-75-0
  QTest::newRow("pos34") << QL1("9780940016750") << 13 << 17 << false; // 978-0-940016-75-0
  QTest::newRow("pos101") << QL1("0446600989;04466") << 0 << 0 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos102") << QL1("0446600989;04466") << 1 << 1 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos103") << QL1("0446600989;04466") << 2 << 3 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos104") << QL1("0446600989;04466") << 3 << 4 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos105") << QL1("0446600989;04466") << 4 << 5 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos106") << QL1("0446600989;04466") << 5 << 7 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos107") << QL1("0446600989;04466") << 6 << 8 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos108") << QL1("0446600989;04466") << 7 << 9 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos109") << QL1("0446600989;04466") << 8 << 10 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos110") << QL1("0446600989;04466") << 9 << 11 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos111") << QL1("0446600989;04466") << 10 << 13 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos112") << QL1("0446600989;04466") << 11 << 15 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos113") << QL1("0446600989;04466") << 12 << 16 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos114") << QL1("0446600989;04466") << 13 << 18 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos115") << QL1("0446600989;04466") << 14 << 19 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos116") << QL1("0446600989;04466") << 15 << 20 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos117") << QL1("0446600989;04466") << 16 << 22 << true; // 0-446-60098-9; 0-4466
  QTest::newRow("pos118") << QL1("0446600989;04466") << 17 << 22 << true; // 0-446-60098-9; 0-4466
}
