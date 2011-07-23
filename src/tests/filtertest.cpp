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
#include "filtertest.moc"
#include "qtest_kde.h"

#include "../filter.h"
#include "../entry.h"

QTEST_KDEMAIN_CORE( FilterTest )

void FilterTest::initTestCase() {
}

void FilterTest::testFilter() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(true, QLatin1String("TestCollection")));
  Tellico::Data::EntryPtr entry(new Tellico::Data::Entry(coll));
  entry->setField(QLatin1String("title"), QLatin1String("Star Wars"));

  Tellico::FilterRule* rule1 = new Tellico::FilterRule(QLatin1String("title"),
                                                       QLatin1String("Star Wars"),
                                                       Tellico::FilterRule::FuncEquals);
  Tellico::Filter filter(Tellico::Filter::MatchAny);
  filter.append(rule1);
  QVERIFY(filter.matches(entry));
  rule1->setFunction(Tellico::FilterRule::FuncNotEquals);
  QVERIFY(!filter.matches(entry));

  rule1->setFunction(Tellico::FilterRule::FuncContains);
  QVERIFY(filter.matches(entry));

  rule1->setFieldName(QLatin1String("author"));
  QVERIFY(!filter.matches(entry));
  rule1->setFunction(Tellico::FilterRule::FuncNotContains);
  QVERIFY(filter.matches(entry));

  rule1->setFieldName(QString());
  rule1->setFunction(Tellico::FilterRule::FuncEquals);
  QVERIFY(filter.matches(entry));

  Tellico::FilterRule* rule2 = new Tellico::FilterRule(QLatin1String("title"),
                                                       QLatin1String("Star"),
                                                       Tellico::FilterRule::FuncEquals);
  filter.clear();
  filter.append(rule2);
  QVERIFY(!filter.matches(entry));

  rule2->setFunction(Tellico::FilterRule::FuncContains);
  QVERIFY(filter.matches(entry));
  rule2->setFunction(Tellico::FilterRule::FuncNotContains);
  QVERIFY(!filter.matches(entry));

  rule2->setFieldName(QLatin1String("author"));
  rule2->setFunction(Tellico::FilterRule::FuncContains);
  QVERIFY(!filter.matches(entry));

  rule2->setFieldName(QString());
  QVERIFY(filter.matches(entry));

  Tellico::FilterRule* rule3 = new Tellico::FilterRule(QLatin1String("title"),
                                                       QLatin1String("Sta[rt]"),
                                                       Tellico::FilterRule::FuncRegExp);
  filter.clear();
  filter.append(rule3);
  QVERIFY(filter.matches(entry));

  rule3->setFunction(Tellico::FilterRule::FuncNotRegExp);
  QVERIFY(!filter.matches(entry));

  rule3->setFieldName(QLatin1String("author"));
  QVERIFY(filter.matches(entry));

  rule3->setFieldName(QString());
  rule3->setFunction(Tellico::FilterRule::FuncRegExp);
  QVERIFY(filter.matches(entry));

  entry->setField(QLatin1String("title"), QString::fromUtf8("Tmavomodrý Svět"));

  Tellico::FilterRule* rule4 = new Tellico::FilterRule(QLatin1String("title"),
                                                       QString::fromUtf8("Tmavomodrý Svět"),
                                                       Tellico::FilterRule::FuncEquals);
  filter.clear();
  filter.append(rule4);
  QVERIFY(filter.matches(entry));

  rule4->setFunction(Tellico::FilterRule::FuncContains);
  QVERIFY(filter.matches(entry));

  rule4->setFunction(Tellico::FilterRule::FuncRegExp);
  QVERIFY(filter.matches(entry));

  Tellico::FilterRule* rule5 = new Tellico::FilterRule(QLatin1String("title"),
                                                       QString::fromUtf8("Tmavomodry Svet"),
                                                       Tellico::FilterRule::FuncEquals);

  filter.clear();
  filter.append(rule5);
  QVERIFY(!filter.matches(entry));

  rule5->setFunction(Tellico::FilterRule::FuncContains);
  QVERIFY(filter.matches(entry));

  rule5->setFunction(Tellico::FilterRule::FuncRegExp);
  QVERIFY(!filter.matches(entry));
}
