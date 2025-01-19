/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "filtertest.h"

#include "../filter.h"
#include "../filterparser.h"
#include "../entry.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../images/imageinfo.h"
#include "../images/imagefactory.h"

#include <KLocalizedString>

#include <QTest>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( FilterTest )

void FilterTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::ImageFactory::init();
}

void FilterTest::testFilter() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true, QStringLiteral("TestCollection")));
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField(QStringLiteral("title"), QStringLiteral("Star Wars"));

  Tellico::FilterRule* rule1 = new Tellico::FilterRule(QStringLiteral("title"),
                                                       QStringLiteral("Star Wars"),
                                                       Tellico::FilterRule::FuncEquals);
  QCOMPARE(rule1->fieldName(), QStringLiteral("title"));
  QCOMPARE(rule1->pattern(), QStringLiteral("Star Wars"));
  QCOMPARE(rule1->function(), Tellico::FilterRule::FuncEquals);

  Tellico::Filter filter(Tellico::Filter::MatchAny);
  filter.append(rule1);
  QVERIFY(filter.matches(entry));
  rule1->setFunction(Tellico::FilterRule::FuncNotEquals);
  QVERIFY(!filter.matches(entry));

  rule1->setFunction(Tellico::FilterRule::FuncContains);
  QVERIFY(filter.matches(entry));

  rule1->setFieldName(QStringLiteral("author"));
  QVERIFY(!filter.matches(entry));
  rule1->setFunction(Tellico::FilterRule::FuncNotContains);
  QVERIFY(filter.matches(entry));

  rule1->setFieldName(QString());
  rule1->setFunction(Tellico::FilterRule::FuncEquals);
  QVERIFY(filter.matches(entry));

  Tellico::FilterRule* rule2 = new Tellico::FilterRule(QStringLiteral("title"),
                                                       QStringLiteral("Star"),
                                                       Tellico::FilterRule::FuncEquals);
  filter.clear();
  filter.append(rule2);
  QVERIFY(!filter.matches(entry));

  rule2->setFunction(Tellico::FilterRule::FuncContains);
  QVERIFY(filter.matches(entry));
  rule2->setFunction(Tellico::FilterRule::FuncNotContains);
  QVERIFY(!filter.matches(entry));

  rule2->setFieldName(QStringLiteral("author"));
  rule2->setFunction(Tellico::FilterRule::FuncContains);
  QVERIFY(!filter.matches(entry));

  rule2->setFieldName(QString());
  QVERIFY(filter.matches(entry));

  Tellico::FilterRule* rule3 = new Tellico::FilterRule(QStringLiteral("title"),
                                                       QStringLiteral("Sta[rt]"),
                                                       Tellico::FilterRule::FuncRegExp);
  QCOMPARE(rule3->pattern(), QStringLiteral("Sta[rt]"));
  filter.clear();
  filter.append(rule3);
  QVERIFY(filter.matches(entry));

  rule3->setFunction(Tellico::FilterRule::FuncNotRegExp);
  QVERIFY(!filter.matches(entry));

  rule3->setFieldName(QStringLiteral("author"));
  QVERIFY(filter.matches(entry));

  rule3->setFieldName(QString());
  rule3->setFunction(Tellico::FilterRule::FuncRegExp);
  QVERIFY(filter.matches(entry));

  entry->setField(QStringLiteral("title"), QStringLiteral("Tmavomodrý Svět"));

  Tellico::FilterRule* rule4 = new Tellico::FilterRule(QStringLiteral("title"),
                                                       QStringLiteral("Tmavomodrý Svět"),
                                                       Tellico::FilterRule::FuncEquals);
  filter.clear();
  filter.append(rule4);
  QVERIFY(filter.matches(entry));

  rule4->setFunction(Tellico::FilterRule::FuncContains);
  QVERIFY(filter.matches(entry));

  rule4->setFunction(Tellico::FilterRule::FuncRegExp);
  QVERIFY(filter.matches(entry));

  Tellico::FilterRule* rule5 = new Tellico::FilterRule(QStringLiteral("title"),
                                                       QStringLiteral("Tmavomodry Svet"),
                                                       Tellico::FilterRule::FuncEquals);

  filter.clear();
  filter.append(rule5);
  QVERIFY(!filter.matches(entry));

  rule5->setFunction(Tellico::FilterRule::FuncContains);
  QVERIFY(filter.matches(entry));

  rule5->setFunction(Tellico::FilterRule::FuncRegExp);
  QVERIFY(!filter.matches(entry));

  Tellico::Data::FieldPtr date(new Tellico::Data::Field(QStringLiteral("date"),
                                                        QStringLiteral("Date"),
                                                        Tellico::Data::Field::Date));
  coll->addField(date);

  Tellico::FilterRule* rule6 = new Tellico::FilterRule(QStringLiteral("date"),
                                                       QStringLiteral("2011-01-24"),
                                                       Tellico::FilterRule::FuncAfter);
  QCOMPARE(rule6->pattern(), QStringLiteral("2011-01-24"));
  filter.clear();
  filter.append(rule6);
  // test Bug 361625
  entry->setField(QStringLiteral("date"), QStringLiteral("2011-1-25"));
  QVERIFY(filter.matches(entry));
  entry->setField(QStringLiteral("date"), QStringLiteral("2011-01-25"));
  QVERIFY(filter.matches(entry));

  rule6->setFunction(Tellico::FilterRule::FuncBefore);
  QVERIFY(!filter.matches(entry));

  // check that a date match is neither before or after
  entry->setField(QStringLiteral("date"), rule6->pattern());
  rule6->setFunction(Tellico::FilterRule::FuncAfter);
  QVERIFY(!filter.matches(entry));
  rule6->setFunction(Tellico::FilterRule::FuncBefore);
  QVERIFY(!filter.matches(entry));

  // check that an invalid date never matches
  entry->setField(QStringLiteral("date"), QStringLiteral("test"));
  rule6->setFunction(Tellico::FilterRule::FuncAfter);
  QVERIFY(!filter.matches(entry));
  rule6->setFunction(Tellico::FilterRule::FuncBefore);
  QVERIFY(!filter.matches(entry));

  Tellico::Data::FieldPtr number(new Tellico::Data::Field(QStringLiteral("number"),
                                                          QStringLiteral("Number"),
                                                          Tellico::Data::Field::Number));
  coll->addField(number);
  entry->setField(QStringLiteral("number"), QStringLiteral("3"));

  Tellico::FilterRule* rule7 = new Tellico::FilterRule(QStringLiteral("number"),
                                                       QStringLiteral("5.0"),
                                                       Tellico::FilterRule::FuncLess);
  QCOMPARE(rule7->pattern(), QStringLiteral("5.0"));
  filter.clear();
  filter.append(rule7);
  QVERIFY(filter.matches(entry));

  rule7->setFunction(Tellico::FilterRule::FuncGreater);
  QVERIFY(!filter.matches(entry));

  entry->setField(QStringLiteral("number"), QStringLiteral("6"));
  QVERIFY(filter.matches(entry));

  // check that a rating can use greater than
  Tellico::Data::FieldPtr rating(new Tellico::Data::Field(QStringLiteral("rating"),
                                                          QStringLiteral("Rating"),
                                                          Tellico::Data::Field::Rating));
  coll->addField(rating);
  entry->setField(QStringLiteral("rating"), QStringLiteral("3"));

  Tellico::FilterRule* rule8 = new Tellico::FilterRule(QStringLiteral("rating"),
                                                       QStringLiteral("2.0"),
                                                       Tellico::FilterRule::FuncGreater);
  QCOMPARE(rule8->pattern(), QStringLiteral("2.0"));
  filter.clear();
  filter.append(rule8);
  QVERIFY(filter.matches(entry));

  rule8->setFunction(Tellico::FilterRule::FuncLess);
  QVERIFY(!filter.matches(entry));

  entry->setField(QStringLiteral("rating"), QStringLiteral("1"));
  QVERIFY(filter.matches(entry));

  // check image size comparisons
  Tellico::Data::FieldPtr imageField(new Tellico::Data::Field(QStringLiteral("image"),
                                                              QStringLiteral("image"),
                                                              Tellico::Data::Field::Image));
  coll->addField(imageField);
  const QString imageName(QStringLiteral("image.png"));
  entry->setField(QStringLiteral("image"), imageName);
  // insert image size into cache (128x128)
  Tellico::Data::ImageInfo imageInfo(imageName, "PNG", 128, 96, false);
  Tellico::ImageFactory::cacheImageInfo(imageInfo);

  Tellico::FilterRule* rule9 = new Tellico::FilterRule(QStringLiteral("image"),
                                                       QStringLiteral("96"),
                                                       Tellico::FilterRule::FuncGreater);
  QVERIFY(!rule9->isEmpty());
  QCOMPARE(rule9->pattern(), QStringLiteral("96"));
  filter.clear();
  filter.append(rule9);
  // compares against larger image dimension, so 128 > 96 matches
  QVERIFY(filter.matches(entry));

  // compares against larger image dimension, so 128 < 96 fails
  rule9->setFunction(Tellico::FilterRule::FuncLess);
  QVERIFY(!filter.matches(entry));

  Tellico::Data::ImageInfo imageInfo2(imageName, "PNG", 96, 96, false);
  Tellico::ImageFactory::cacheImageInfo(imageInfo2);

  rule9->setFunction(Tellico::FilterRule::FuncLess);
  QVERIFY(!filter.matches(entry));
  rule9->setFunction(Tellico::FilterRule::FuncEquals);
  QVERIFY(filter.matches(entry));
  // an empty image should also match less than size
  entry->setField(QStringLiteral("image"), QString());
  rule9->setFunction(Tellico::FilterRule::FuncLess);
  QVERIFY(filter.matches(entry));

  // test a filter for matching against an empty string
  Tellico::Data::FieldPtr testField(new Tellico::Data::Field(QStringLiteral("test"),
                                                             QStringLiteral("Test")));
  coll->addField(testField);
  Tellico::FilterRule* rule10 = new Tellico::FilterRule(QStringLiteral("test"),
                                                        QString(),
                                                        Tellico::FilterRule::FuncEquals);
  QVERIFY(!rule10->isEmpty());
  filter.clear();
  filter.append(rule10);
  QVERIFY(filter.matches(entry));
  rule10->setFunction(Tellico::FilterRule::FuncNotEquals);
  QVERIFY(!rule10->isEmpty());
  QVERIFY(!filter.matches(entry));
}

void FilterTest::testGroupViewFilter() {
  // ideally, I'd instantiate a GroupView object and test that, but it's tough with all the dependencies
  // so this code is identical to what is in Tellico::GroupView::slotFilterGroup()
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true, QStringLiteral("TestCollection")));
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QStringLiteral("author"), QStringLiteral("John Author"));
  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  entry2->setField(QStringLiteral("author"), QStringLiteral("John Q. Author"));
  Tellico::Data::EntryPtr entry3(new Tellico::Data::Entry(coll));
  entry3->setField(QStringLiteral("author"), QStringLiteral("John Author") +
                                            Tellico::FieldFormat::delimiterString() +
                                            QStringLiteral("James Author"));
  Tellico::Data::EntryPtr entry4(new Tellico::Data::Entry(coll));
  entry4->setField(QStringLiteral("author"), QStringLiteral("James Author") +
                                            Tellico::FieldFormat::delimiterString() +
                                            QStringLiteral("John Author"));
  Tellico::Data::EntryPtr entry5(new Tellico::Data::Entry(coll));
  entry5->setField(QStringLiteral("author"), QStringLiteral("James Author") +
                                            Tellico::FieldFormat::delimiterString() +
                                            QStringLiteral("John Q. Author"));

  QString pattern(entry1->formattedField(QStringLiteral("author")));
  // the filter should match all since it was the initial way the group view filter was constructed
  Tellico::Filter filter1(Tellico::Filter::MatchAny);
  filter1.append(new Tellico::FilterRule(QStringLiteral("author"), pattern, Tellico::FilterRule::FuncContains));
  QVERIFY(filter1.matches(entry1));
  QVERIFY(filter1.matches(entry2));
  QVERIFY(filter1.matches(entry3));
  QVERIFY(filter1.matches(entry4));
  QVERIFY(filter1.matches(entry5));

  QString rxPattern = Tellico::FieldFormat::matchValueRegularExpression(pattern);
  // the filter should match entry1, entry3, and entry 4 but not entry2 or entry5
  Tellico::Filter filter2(Tellico::Filter::MatchAny);
  filter2.append(new Tellico::FilterRule(QStringLiteral("author"), rxPattern, Tellico::FilterRule::FuncRegExp));
  QVERIFY(filter2.matches(entry1));
  QVERIFY(!filter2.matches(entry2)); // does not match
  QVERIFY(filter2.matches(entry3));
  QVERIFY(filter2.matches(entry4));
  QVERIFY(!filter2.matches(entry5));

  // Bug 415886
  Tellico::Data::CollPtr coll2(new Tellico::Data::VideoCollection(true, QStringLiteral("TestCollection2")));
  Tellico::Data::EntryPtr movie(new Tellico::Data::Entry(coll2));
  movie->setField(QStringLiteral("cast"), QStringLiteral("John Author") +
                                          Tellico::FieldFormat::columnDelimiterString() +
                                          QStringLiteral("role"));
  Tellico::Filter castFilter(Tellico::Filter::MatchAny);
  castFilter.append(new Tellico::FilterRule(QStringLiteral("cast"), rxPattern, Tellico::FilterRule::FuncRegExp));
  // single table row with value
  QVERIFY(castFilter.matches(movie));
  movie->setField(QStringLiteral("cast"), QStringLiteral("John Author") +
                                          Tellico::FieldFormat::rowDelimiterString() +
                                          QStringLiteral("Second Author"));
  // multiple table row with value only
  QVERIFY(castFilter.matches(movie));
  movie->setField(QStringLiteral("cast"), QStringLiteral("No one") +
                                          Tellico::FieldFormat::rowDelimiterString() +
                                          QStringLiteral("John Author") +
                                          Tellico::FieldFormat::rowDelimiterString() +
                                          QStringLiteral("Second Author"));
  // multiple table row with value second
  QVERIFY(castFilter.matches(movie));
}

void FilterTest::testFilterParser() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true, QStringLiteral("TestCollection")));
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField(QStringLiteral("title"), QStringLiteral("C++ Coding Standards"));
  entry->setField(QStringLiteral("author"), QStringLiteral("Herb Sutter"));

  Tellico::FilterParser parser(QStringLiteral("C++"), true /* allow regexps */);
  Tellico::FilterPtr filter = parser.filter();
  QVERIFY(filter->matches(entry));

  entry->setField(QStringLiteral("title"), QStringLiteral("Coding Standards"));
  QVERIFY(filter->matches(entry)); // still matches due to c++ being interpreted as a regexp

  Tellico::FilterParser parser2(QStringLiteral("C++"), false /* allow regexps */);
  filter = parser2.filter();
  QVERIFY(!filter->matches(entry)); // no longer matches

  entry->setField(QStringLiteral("title"), QStringLiteral("C++ Coding Standards"));
  QVERIFY(filter->matches(entry));

  // match title field only
  Tellico::FilterParser parser3(QStringLiteral("title=C++"), false /* allow regexps */);
  filter = parser3.filter();
  auto rule0 = filter->at(0);
  QVERIFY(rule0);
  QCOMPARE(rule0->fieldName(), QLatin1String("title"));
  QCOMPARE(rule0->pattern(), QLatin1String("C++"));

  entry->setField(QStringLiteral("title"), QStringLiteral("C++ Coding Standards"));
  QVERIFY(filter->matches(entry));

  // shouldn't match since there's no field names Title2
  Tellico::FilterParser parser4(QStringLiteral("Title2=C++"), false /* allow regexps */);
  parser4.setCollection(coll);
  filter = parser4.filter();
  QVERIFY(!filter->matches(entry));

  // match by field title instead
  Tellico::FilterParser parser5(QStringLiteral("Title=C++"), false /* allow regexps */);
  parser5.setCollection(coll);
  filter = parser5.filter();
  QVERIFY(filter->matches(entry));

  Tellico::FilterParser parser6(QStringLiteral("title=\"C++ coding\""), false /* allow regexps */);
  filter = parser6.filter();
  rule0 = filter->at(0);
  QVERIFY(rule0);
  QCOMPARE(rule0->fieldName(), QLatin1String("title"));
  QCOMPARE(rule0->pattern(), QLatin1String("C++ coding"));

  Tellico::FilterParser parser7(QStringLiteral("title=\"C++ coding\" author=sutter"), false /* allow regexps */);
  filter = parser7.filter();
  rule0 = filter->at(0);
  QVERIFY(rule0);
  QCOMPARE(rule0->fieldName(), QLatin1String("title"));
  QCOMPARE(rule0->pattern(), QLatin1String("C++ coding"));
  QVERIFY(filter->size() > 1);
  auto rule1 = filter->at(1);
  QVERIFY(rule1);
  QCOMPARE(rule1->fieldName(), QLatin1String("author"));
  QCOMPARE(rule1->pattern(), QLatin1String("sutter"));
}
