/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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


#include "fieldwidgettest.h"
#include "../gui/boolfieldwidget.h"
#include "../gui/urlfieldwidget.h"
#include "../document.h"
#include "../images/imagefactory.h"

#include <KUrlRequester>

#include <QTest>
#include <QCheckBox>

// needs a GUI
QTEST_MAIN( FieldWidgetTest )

void FieldWidgetTest::initTestCase() {
  Tellico::ImageFactory::init();
}

void FieldWidgetTest::testBool() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("bool"),
                                                         QStringLiteral("bool"),
                                                         Tellico::Data::Field::Bool));
  Tellico::GUI::BoolFieldWidget w(field, nullptr);
  QVERIFY(w.text().isEmpty());
  w.setTextImpl(QStringLiteral("true"));
  QCOMPARE(w.text(), QStringLiteral("true"));
  QCheckBox* cb = dynamic_cast<QCheckBox*>(w.widget());
  QVERIFY(cb);
  QVERIFY(cb->isChecked());

  // any non-empty text is interpreted as true (for better or worse)
  w.setTextImpl(QStringLiteral("false"));
  QCOMPARE(w.text(), QStringLiteral("true"));
  QVERIFY(cb->isChecked());

  w.clearImpl();
  QVERIFY(w.text().isEmpty());
  QVERIFY(!cb->isChecked());
}

void FieldWidgetTest::testUrl() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("url"),
                                                         QStringLiteral("url"),
                                                         Tellico::Data::Field::URL));
  Tellico::GUI::URLFieldWidget w(field, nullptr);

  QUrl base = QUrl::fromLocalFile(QFINDTESTDATA("data/relative-link.xml"));
  Tellico::Data::Document::self()->setURL(base); // set the base url
  QUrl link = QUrl::fromLocalFile(QFINDTESTDATA("fieldwidgettest.cpp"));
  w.m_requester->setUrl(link);
  QCOMPARE(w.text(), link.url());

  field->setProperty(QStringLiteral("relative"), QStringLiteral("true"));
  w.updateFieldHook(field, field);
  // will be exactly up one level
  QCOMPARE(w.text(), QStringLiteral("../fieldwidgettest.cpp"));

// verify value after setting the relative link explicitly
  w.setTextImpl(QStringLiteral("../fieldwidgettest.cpp"));
  QCOMPARE(w.text(), QStringLiteral("../fieldwidgettest.cpp"));

  field->setProperty(QStringLiteral("relative"), QStringLiteral("false"));
  w.updateFieldHook(field, field);
  // will be exactly up one level
  QCOMPARE(w.text(), link.url());

  w.clear();
  QVERIFY(w.text().isEmpty());
  QVERIFY(w.m_requester->url().isEmpty());
}
