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

#include "qtest_kde.h"
#include "isbntest.h"
#include "isbntest.moc"
#include "../isbnvalidator.h"

QTEST_KDEMAIN_CORE( IsbnTest )

QString fixup(const char* s) {
  static const Tellico::ISBNValidator val(0);
  QString qs = QLatin1String(s);
  val.fixup(qs);
  return qs;
}

void IsbnTest::testGarbage() {
  QCOMPARE(fixup("My name is robby"), QString());
  QCOMPARE(fixup("http://www.abclinuxu.cz/clanky/show/63080"), QLatin1String("6-3080"));
}

void IsbnTest::testFormat() {
  // initial checks
  QCOMPARE(fixup("0-446-60098-9"), QLatin1String("0-446-60098-9"));
  // check sum value
  QCOMPARE(fixup("0-446-60098"), QLatin1String("0-446-60098-9"));

  QCOMPARE(Tellico::ISBNValidator::isbn10("978-0-06-087298-4"), QLatin1String("0-06-087298-5"));
  QCOMPARE(Tellico::ISBNValidator::isbn13("0-06-087298-5"), QLatin1String("978-0-06-087298-4"));

  // check EAN-13
  QCOMPARE(fixup("9780940016750"), QLatin1String("978-0-940016-75-0"));
  QCOMPARE(fixup("978-0940016750"), QLatin1String("978-0-940016-75-0"));
  QCOMPARE(fixup("978-0-940016-75-0"), QLatin1String("978-0-940016-75-0"));
  QCOMPARE(fixup("978286274486"), QLatin1String("978-2-86274-486-5"));
  QCOMPARE(fixup("9788186119130"), QLatin1String("978-81-86-11913-6"));
  QCOMPARE(fixup("9788186119137"), QLatin1String("978-81-86-11913-6"));
  QCOMPARE(fixup("97881-8611-9-13-0"), QLatin1String("978-81-86-11913-6"));
  QCOMPARE(fixup("97881-8611-9-13-7"), QLatin1String("978-81-86-11913-6"));

  // don't add checksum for EAN that start with 978 or 979 and are less than 13 in length
  QCOMPARE(fixup("978059600"), QLatin1String("978-059600"));
  QCOMPARE(fixup("978-0596000"), QLatin1String("978-059600-0"));
}

void IsbnTest::testHyphenation() {
  // normal english-language hyphenation
  QCOMPARE(fixup("0-596-00053"), QLatin1String("0-596-00053-7"));
  QCOMPARE(fixup("044660098"), QLatin1String("0-446-60098-9"));
  QCOMPARE(fixup("0446600989"), QLatin1String("0-446-60098-9"));

  // check french hyphenation
  QCOMPARE(fixup("2862744867"), QLatin1String("2-86274-486-7"));

  // check german hyphenation
  QCOMPARE(fixup("3423071516"), QLatin1String("3-423-07151-6"));

  // check keeping middle hyphens
  QCOMPARE(fixup("6-18611913-0"),   QLatin1String("6-18611913-0"));
  QCOMPARE(fixup("6-186119-13-0"),  QLatin1String("6-186119-13-0"));
  QCOMPARE(fixup("6-18611-9-13-0"), QLatin1String("6-18611-913-0"));
}
