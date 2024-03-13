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

#include "fieldtest.h"

#include "../field.h"
#include "../utils/urlfieldlogic.h"

#include <KLocalizedString>

#include <QTest>

QTEST_APPLESS_MAIN( FieldTest )

void FieldTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
}

void FieldTest::testAll() {
  Tellico::Data::Field field1(QStringLiteral("name"), QStringLiteral("title"));

  // the default field type is line
  QCOMPARE(field1.type(), Tellico::Data::Field::Line);
  QCOMPARE(field1.name(), QStringLiteral("name"));
  QCOMPARE(field1.title(), QStringLiteral("title"));
  // description should match title, to begin with
  QCOMPARE(field1.description(), QStringLiteral("title"));
  QCOMPARE(field1.flags(), 0);
  QCOMPARE(field1.formatType(), Tellico::FieldFormat::FormatNone);
  field1.setTitle(QStringLiteral("newtitle"));
  QCOMPARE(field1.title(), QStringLiteral("newtitle"));
  field1.setCategory(QStringLiteral("category"));
  QCOMPARE(field1.category(), QStringLiteral("category"));
  field1.setDefaultValue(QStringLiteral("default"));
  QCOMPARE(field1.defaultValue(), QStringLiteral("default"));
  QCOMPARE(field1.isSingleCategory(), false);

  Tellico::Data::Field field1a = field1;
  QCOMPARE(field1.type(), field1a.type());
  QCOMPARE(field1.category(), field1a.category());
  QCOMPARE(field1.propertyList(), field1a.propertyList());

  Tellico::Data::Field field2(QStringLiteral("table"), QStringLiteral("Table"), Tellico::Data::Field::Table2);
  // should be converted to Table type
  QCOMPARE(field2.type(), Tellico::Data::Field::Table);
  QCOMPARE(field2.property(QStringLiteral("columns")), QStringLiteral("2"));
  field2.setProperty(QStringLiteral("columns"), QStringLiteral("1"));
  QCOMPARE(field2.property(QStringLiteral("columns")), QStringLiteral("1"));
  QCOMPARE(field2.isSingleCategory(), true);
  QCOMPARE(field2.flags(), int(Tellico::Data::Field::AllowMultiple));
  QVERIFY(field2.hasFlag(Tellico::Data::Field::AllowMultiple));

  QStringList allowed;
  allowed << QStringLiteral("choice1");
  Tellico::Data::Field field3(QStringLiteral("choice"), QStringLiteral("Choice"), allowed);
  QCOMPARE(field3.type(), Tellico::Data::Field::Choice);
  QCOMPARE(field3.allowed(), allowed);
  field3.setDefaultValue(QStringLiteral("default"));
  QCOMPARE(field3.defaultValue(), QString());
  QCOMPARE(field3.flags(), 0);

  Tellico::Data::Field field4(QStringLiteral("derived"), QStringLiteral("Derived"), Tellico::Data::Field::Dependent);
  // should be converted to Line type
  QCOMPARE(field4.type(), Tellico::Data::Field::Line);
  QCOMPARE(field4.isSingleCategory(), false);
  QCOMPARE(field4.flags(), int(Tellico::Data::Field::Derived));
  QVERIFY(field4.hasFlag(Tellico::Data::Field::Derived));

  Tellico::Data::Field field5(QStringLiteral("readonly"), QStringLiteral("Readonly"), Tellico::Data::Field::ReadOnly);
  // should be converted to Line type
  QCOMPARE(field5.type(), Tellico::Data::Field::Line);
  QCOMPARE(field5.isSingleCategory(), false);
  QCOMPARE(field5.flags(), int(Tellico::Data::Field::NoEdit));
  QVERIFY(field5.hasFlag(Tellico::Data::Field::NoEdit));
}

void FieldTest::testUrlFieldLogic() {
  Tellico::UrlFieldLogic logic;
  // starts out with absolute urls
  QCOMPARE(logic.isRelative(), false);

  // use a local data file to test
  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("data/test.ris"));
  // since the logic is still absolute, the url should be unchanged
  QCOMPARE(logic.urlText(u), u.url());

  logic.setRelative(true);
  // since the base url is not set, the url should be unchanged
  QCOMPARE(logic.urlText(u), u.url());

  logic.setBaseUrl(QUrl(QStringLiteral("http://tellico-project.org")));
  // since the base url is not local, the url should be unchanged
  QCOMPARE(logic.urlText(u), u.url());

  // now use the local parent directory
  QUrl base = u.adjusted(QUrl::RemoveFilename);
  logic.setBaseUrl(base);
  // url text should just be the file name since it's in the same folder
  QCOMPARE(logic.urlText(u), QStringLiteral("test.ris"));

  // the base url is actually a tellico data file
  base = QUrl::fromLocalFile(QFINDTESTDATA("data/with-image.tc"));
  logic.setBaseUrl(base);
  // url text should still be the file name since it's in the same folder
  QCOMPARE(logic.urlText(u), QStringLiteral("test.ris"));

  // check a relative file one folder deep
  u = QUrl::fromLocalFile(QFINDTESTDATA("data/alexandria/0060574623.yaml"));
  QCOMPARE(logic.urlText(u), QStringLiteral("alexandria/0060574623.yaml"));

  logic.setRelative(false);
  QCOMPARE(logic.urlText(u), u.url());

  // test relative to url for unknown document
  logic.setBaseUrl(QUrl(QStringLiteral("file:Unknown")));
  logic.setRelative(true);
  // logic result should not be a relative path, but full path
  QCOMPARE(logic.urlText(u), u.url());
}

void FieldTest::testFieldTypeChange() {
  Tellico::Data::Field field1(QStringLiteral("name"), QStringLiteral("title"));

  // the default field type is line
  QCOMPARE(field1.type(), Tellico::Data::Field::Line);
  QCOMPARE(field1.name(), QStringLiteral("name"));
  QCOMPARE(field1.title(), QStringLiteral("title"));
  // description should match title, to begin with
  QCOMPARE(field1.description(), QStringLiteral("title"));
  QCOMPARE(field1.flags(), 0);
  QCOMPARE(field1.formatType(), Tellico::FieldFormat::FormatNone);
  field1.setCategory(QStringLiteral("category"));
  QCOMPARE(field1.category(), QStringLiteral("category"));
  QCOMPARE(field1.isSingleCategory(), false);

  // change title to a table
  field1.setType(Tellico::Data::Field::Table);

  QCOMPARE(field1.type(), Tellico::Data::Field::Table);
  QCOMPARE(field1.name(), QStringLiteral("name"));
  QVERIFY(field1.hasFlag(Tellico::Data::Field::AllowMultiple));
  QCOMPARE(field1.formatType(), Tellico::FieldFormat::FormatNone);
  // category is now same as the title
  QCOMPARE(field1.category(), QStringLiteral("title"));
  QVERIFY(field1.isSingleCategory());

  QCOMPARE(Tellico::Data::Field::typeMap().count(),
           Tellico::Data::Field::typeTitles().count());
}
