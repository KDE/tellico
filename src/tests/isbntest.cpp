/***************************************************************************
    copyright            : (C) 2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#undef QT_NO_CAST_FROM_ASCII

#include "qtest_kde.h"
#include "isbntest.h"
#include "isbntest.moc"
#include "../utils/isbnvalidator.h"

QTEST_KDEMAIN_CORE( IsbnTest )

#define QL1(x) QString::fromLatin1(x)

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
  QTest::newRow("0-596-00053") << QL1("0-596-00053") << QL1("0-596-00053-7");
  QTest::newRow("044660098") << QL1("044660098") << QL1("0-446-60098-9");
  QTest::newRow("0446600989") << QL1("0446600989") << QL1("0-446-60098-9");

  // check french hyphenation
  QTest::newRow("2862744867") << QL1("2862744867") << QL1("2-86274-486-7");

  // check german hyphenation
  QTest::newRow("3423071516") << QL1("3423071516") << QL1("3-423-07151-6");

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
}

void IsbnTest::testListDifference() {
  QStringList list1;
  list1 << QLatin1String("0940016753") << QLatin1String("9780940016750");
  QStringList list2;

  // comparing to empty list should return the first list
  QCOMPARE(Tellico::ISBNValidator::listDifference(list1, list2), list1);

  // comparing to a value that matches everything in the list should return empty list
  list2 << QLatin1String("0-940016-75-0");
  QCOMPARE(Tellico::ISBNValidator::listDifference(list1, list2), QStringList());
}
