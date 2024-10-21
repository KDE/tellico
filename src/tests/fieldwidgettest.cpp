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
#include "../gui/choicefieldwidget.h"
#include "../gui/datefieldwidget.h"
#include "../gui/datewidget.h"
#include "../gui/linefieldwidget.h"
#include "../gui/lineedit.h"
#include "../gui/numberfieldwidget.h"
#include "../gui/spinbox.h"
#include "../gui/parafieldwidget.h"
#include "../gui/ratingwidget.h"
#include "../gui/ratingfieldwidget.h"
#include "../gui/tablefieldwidget.h"
#include "../gui/urlfieldwidget.h"
#include "../document.h"
#include "../images/imagefactory.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"

#include <KLocalizedString>
#include <KTextEdit>
#include <KUrlRequester>

#include <QTest>
#include <QCheckBox>
#include <QComboBox>
#include <QTableWidget>
#include <QSignalSpy>
#include <QStandardPaths>

// needs a GUI
QTEST_MAIN( FieldWidgetTest )

void FieldWidgetTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::ImageFactory::init();
}

void FieldWidgetTest::testBool() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("bool"),
                                                         QStringLiteral("bool"),
                                                         Tellico::Data::Field::Bool));
  field->setDefaultValue(QStringLiteral("true"));
  Tellico::GUI::BoolFieldWidget w(field, nullptr);
  QSignalSpy spy(&w, &Tellico::GUI::FieldWidget::valueChanged);
  QVERIFY(!w.expands());
  QVERIFY(w.text().isEmpty());
  QCOMPARE(spy.count(), 0);

  w.setText(QStringLiteral("true"));
  QCOMPARE(w.text(), QStringLiteral("true"));
  QCOMPARE(spy.count(), 0); // since the value was set explicitly, no valueChanged signal is made
  auto cb = dynamic_cast<QCheckBox*>(w.widget());
  QVERIFY(cb);
  QVERIFY(cb->isChecked());

  // any non-empty text is interpreted as true (for better or worse)
  w.setText(QStringLiteral("false"));
  QCOMPARE(w.text(), QStringLiteral("true"));
  QVERIFY(cb->isChecked());

  w.clear();
  QVERIFY(w.text().isEmpty());
  QVERIFY(!cb->isChecked());
  QCOMPARE(spy.count(), 0);

  cb->setChecked(true);
  QCOMPARE(w.text(), QStringLiteral("true"));
  QCOMPARE(spy.count(), 1);

  w.clear();
  QVERIFY(w.text().isEmpty());
  QVERIFY(!cb->isChecked());

  w.insertDefault();
  QCOMPARE(w.text(), QStringLiteral("true"));
  QVERIFY(cb->isChecked());
}

void FieldWidgetTest::testChoice() {
  // create a Choice field
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("f"),
                                                         QStringLiteral("f"),
                                                         QStringList()));
  field->setAllowed(QStringList() << QStringLiteral("choice1"));
  Tellico::GUI::ChoiceFieldWidget w(field, nullptr);
  QSignalSpy spy(&w, &Tellico::GUI::FieldWidget::valueChanged);
  QVERIFY(!w.expands());
  QVERIFY(w.text().isEmpty());
  auto cb = dynamic_cast<QComboBox*>(w.widget());
  QVERIFY(cb);
  QCOMPARE(cb->count(), 2); // one empty value

  field->setAllowed(QStringList() << QStringLiteral("choice1") << QStringLiteral("choice2"));
  w.updateField(field, field);
  QVERIFY(w.text().isEmpty());
  QCOMPARE(spy.count(), 0);
  QCOMPARE(cb->count(), 3);

  w.setText(QStringLiteral("choice2"));
  QCOMPARE(w.text(), QStringLiteral("choice2"));

  field->setAllowed(QStringList() << QStringLiteral("choice1") << QStringLiteral("choice2") << QStringLiteral("choice3"));
  w.updateField(field, field);
  // selected value should remain same
  QCOMPARE(w.text(), QStringLiteral("choice2"));
  QCOMPARE(spy.count(), 0);

  cb->setCurrentIndex(1);
  QCOMPARE(w.text(), QStringLiteral("choice1"));
  QCOMPARE(spy.count(), 1);

  w.clear();
  QVERIFY(w.text().isEmpty());

  // set value to something not in the list
  w.setText(QStringLiteral("choice4"));
  QCOMPARE(w.text(), QStringLiteral("choice4"));
  QCOMPARE(cb->count(), 5);

  w.insertDefault();
  QVERIFY(w.text().isEmpty());
}

void FieldWidgetTest::testDate() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("d"),
                                                         QStringLiteral("d"),
                                                         Tellico::Data::Field::Date));
  Tellico::GUI::DateFieldWidget w(field, nullptr);
  QSignalSpy spy(&w, &Tellico::GUI::FieldWidget::valueChanged);
  QVERIFY(w.expands());
  auto dw = dynamic_cast<Tellico::GUI::DateWidget*>(w.widget());
  QVERIFY(dw);
  QVERIFY(w.text().isEmpty());
  QVERIFY(dw->date().isNull());
  QCOMPARE(spy.count(), 0);

  QDate moon(1969, 7, 20);
  w.setText(QStringLiteral("1969-07-20"));
  QCOMPARE(w.text(), QStringLiteral("1969-07-20"));
  QCOMPARE(dw->date(), moon);
  // test without leading zero
  w.setText(QStringLiteral("1969-7-20"));
  QCOMPARE(w.text(), QStringLiteral("1969-07-20"));
  QCOMPARE(dw->date(), moon);
  QCOMPARE(spy.count(), 0);

  w.setText(QString());
  QVERIFY(w.text().isEmpty());
  QVERIFY(dw->date().isNull());
  QCOMPARE(spy.count(), 0);

  w.setText(QStringLiteral("1969"));
  // adds dashes
  QCOMPARE(w.text(), QStringLiteral("1969--"));
  QVERIFY(dw->date().isNull());
  QCOMPARE(spy.count(), 0);

  QDate sputnik(1957, 10, 4);
  dw->setDate(sputnik);
  QCOMPARE(w.text(), QStringLiteral("1957-10-04"));
  QCOMPARE(dw->date(), sputnik);
  QCOMPARE(spy.count(), 1);

  w.clear();
  QVERIFY(w.text().isEmpty());
  QVERIFY(dw->date().isNull());
}

void FieldWidgetTest::testLine() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("f"),
                                                         QStringLiteral("f")));
  field->setFlags(Tellico::Data::Field::AllowMultiple | Tellico::Data::Field::AllowCompletion);
  Tellico::GUI::LineFieldWidget w(field, nullptr);
  QSignalSpy spy(&w, &Tellico::GUI::FieldWidget::valueChanged);
  QVERIFY(w.expands());
  QVERIFY(w.text().isEmpty());

  w.setText(QStringLiteral("true"));
  QCOMPARE(w.text(), QStringLiteral("true"));
  auto le = dynamic_cast<Tellico::GUI::LineEdit*>(w.widget());
  QVERIFY(le);
  QVERIFY(!le->validator());
  QCOMPARE(spy.count(), 0);

  w.addCompletionObjectItem(QStringLiteral("new text"));
  le->setText(QStringLiteral("new"));
  QCOMPARE(spy.count(), 1);
  QCOMPARE(le->completionObject()->makeCompletion(le->text()), QStringLiteral("new text"));

  le->setText(QStringLiteral("new text"));
  QCOMPARE(w.text(), QStringLiteral("new text"));
  QCOMPARE(spy.count(), 2);

  le->setText(QStringLiteral("text1;text2"));
  QCOMPARE(w.text(), QStringLiteral("text1; text2"));
  QCOMPARE(spy.count(), 3);

  field->setFlags(Tellico::Data::Field::AllowMultiple);
  w.updateField(field, field);
  // verify completion object is removed
  QVERIFY(!le->compObj()); // don't call completionObject() since it recreates it

  w.clear();
  QVERIFY(w.text().isEmpty());
}

void FieldWidgetTest::testPara() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("f"),
                                                         QStringLiteral("f"),
                                                         Tellico::Data::Field::Para));
  Tellico::GUI::ParaFieldWidget w(field, nullptr);
  QSignalSpy spy(&w, &Tellico::GUI::FieldWidget::valueChanged);
  QVERIFY(w.expands());
  QVERIFY(w.text().isEmpty());

  w.setText(QStringLiteral("true"));
  QCOMPARE(w.text(), QStringLiteral("true"));
  auto edit = dynamic_cast<KTextEdit*>(w.widget());
  QVERIFY(edit);
  QCOMPARE(spy.count(), 0);

  // test replacing EOL
  edit->setText(QLatin1String("test1\ntest2"));
  QCOMPARE(w.text(), QStringLiteral("test1<br/>test2"));
  QCOMPARE(spy.count(), 1);

  field->setProperty(QLatin1String("replace-line-feeds"), QLatin1String("false"));
  w.updateField(field, field);
  edit->setText(QLatin1String("test1\ntest2"));
  QCOMPARE(w.text(), QStringLiteral("test1\ntest2"));

  field->setProperty(QLatin1String("replace-line-feeds"), QLatin1String("true"));
  w.updateField(field, field);
  w.setText(QLatin1String("test1<br>test2"));
  QCOMPARE(edit->toPlainText(), QStringLiteral("test1\ntest2"));
  QCOMPARE(w.text(), QStringLiteral("test1<br/>test2"));

  field->setProperty(QLatin1String("replace-line-feeds"), QString());
  w.updateField(field, field);
  w.setText(QLatin1String("test1<br>test2"));
  QCOMPARE(edit->toPlainText(), QStringLiteral("test1\ntest2"));
  QCOMPARE(w.text(), QStringLiteral("test1<br/>test2"));

  w.clear();
  QVERIFY(w.text().isEmpty());

  QString textWithEmoji = QString::fromUtf8("Title 🏡️");
  w.setText(textWithEmoji);
  QCOMPARE(w.text(), textWithEmoji);
}

void FieldWidgetTest::testNumber() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("f"),
                                                         QStringLiteral("f"),
                                                         Tellico::Data::Field::Number));
  Tellico::GUI::NumberFieldWidget w(field, nullptr);
  QSignalSpy spy(&w, &Tellico::GUI::FieldWidget::valueChanged);
  QVERIFY(w.expands());
  QVERIFY(w.text().isEmpty());
  // spin box since AllowMultiple is not set
  QVERIFY(w.isSpinBox());
  auto sb = dynamic_cast<Tellico::GUI::SpinBox*>(w.widget());
  QVERIFY(sb);
  w.setText(QStringLiteral("1"));
  QCOMPARE(w.text(), QStringLiteral("1"));
  QCOMPARE(sb->value(), 1);
  w.setText(QStringLiteral("1; 2"));
  QCOMPARE(w.text(), QStringLiteral("1"));
  w.clear();
  QVERIFY(w.text().isEmpty());
  QCOMPARE(spy.count(), 0);

  sb->setValue(3);
  QCOMPARE(w.text(), QStringLiteral("3"));
  QCOMPARE(spy.count(), 1);

  // now set AllowMultiple and check that the spinbox is deleted and a line edit is used
  field->setFlags(Tellico::Data::Field::AllowMultiple);
  w.setText(QStringLiteral("1"));
  w.updateField(field, field);
  QVERIFY(!w.isSpinBox());
  auto le = dynamic_cast<QLineEdit*>(w.widget());
  QVERIFY(le);
  // value should be unchanged
  QCOMPARE(w.text(), QStringLiteral("1"));
  QCOMPARE(spy.count(), 1);
  w.setText(QStringLiteral("1;2"));
  QCOMPARE(w.text(), QStringLiteral("1; 2"));
  QCOMPARE(spy.count(), 1);

  le->setText(QStringLiteral("2"));
  QCOMPARE(w.text(), QStringLiteral("2"));
  QCOMPARE(spy.count(), 2);

  w.clear();
  QVERIFY(w.text().isEmpty());
}

void FieldWidgetTest::testRating() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("f"),
                                                         QStringLiteral("f"),
                                                         Tellico::Data::Field::Rating));
  Tellico::GUI::RatingFieldWidget w(field, nullptr);
  QSignalSpy spy(&w, &Tellico::GUI::FieldWidget::valueChanged);
  QVERIFY(!w.expands());
  QVERIFY(w.text().isEmpty());
  auto rating = dynamic_cast<Tellico::GUI::RatingWidget*>(w.widget());
  QVERIFY(rating);

  w.setText(QStringLiteral("1"));
  QCOMPARE(w.text(), QStringLiteral("1"));
  w.setText(QStringLiteral("1; 2"));
  QCOMPARE(w.text(), QStringLiteral("1"));
  w.clear();
  QVERIFY(w.text().isEmpty());
  QCOMPARE(spy.count(), 0);

  field->setProperty(QStringLiteral("minimum"), QStringLiteral("5"));
  field->setProperty(QStringLiteral("maximum"), QStringLiteral("7"));
  w.setText(QStringLiteral("4"));
  w.updateField(field, field);
  QVERIFY(w.text().isEmpty()); // empty since 4 is less than minimum
  QCOMPARE(spy.count(), 0);
  w.setText(QStringLiteral("8"));
  QVERIFY(w.text().isEmpty());
  QCOMPARE(spy.count(), 0);

  rating->setText(QStringLiteral("6"));
  QCOMPARE(w.text(), QStringLiteral("6"));
  QCOMPARE(spy.count(), 0);
}

void FieldWidgetTest::testTable() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("url"),
                                                         QStringLiteral("url"),
                                                         Tellico::Data::Field::Table));
  field->setProperty(QStringLiteral("columns"), QStringLiteral("2"));
  Tellico::GUI::TableFieldWidget w(field, nullptr);
  QSignalSpy spy(&w, &Tellico::GUI::FieldWidget::valueChanged);
  QSignalSpy fieldSpy(&w, &Tellico::GUI::FieldWidget::fieldChanged);
  QVERIFY(w.expands());
  QVERIFY(w.text().isEmpty());
  QCOMPARE(w.m_columns, 2);

  auto tw = dynamic_cast<QTableWidget*>(w.widget());
  Q_ASSERT(tw);
  QCOMPARE(tw->columnCount(), 2);
  QCOMPARE(tw->rowCount(), 5); // minimum row count is 5

  w.setText(QStringLiteral("true"));
  QCOMPARE(w.text(), QStringLiteral("true"));
  QCOMPARE(spy.count(), 0);
  QCOMPARE(tw->rowCount(), 5);

  w.slotInsertRow();
  tw->setItem(0, 1, new QTableWidgetItem(QStringLiteral("new text")));
  QCOMPARE(w.text(), QStringLiteral("true::new text"));
  QCOMPARE(spy.count(), 1);
  QVERIFY(!w.emptyRow(0));
  QCOMPARE(tw->rowCount(), 5);

  w.m_row = 1;
  w.slotInsertRow();
  QCOMPARE(tw->rowCount(), 6);
  w.slotRemoveRow();
  QCOMPARE(w.text(), QStringLiteral("true::new text"));
  QCOMPARE(tw->rowCount(), 5);
  QCOMPARE(spy.count(), 1);
  QVERIFY(w.emptyRow(1));
  QVERIFY(!w.emptyRow(0));

  QCOMPARE(w.text(), QStringLiteral("true::new text"));
  w.m_row = 0;
  w.slotMoveRowDown();
  QCOMPARE(spy.count(), 2);
  QCOMPARE(w.text(), Tellico::FieldFormat::rowDelimiterString() + QStringLiteral("true::new text"));
  w.m_row = 1;
  w.slotMoveRowUp();
  QCOMPARE(spy.count(), 3);
  QCOMPARE(w.text(), QStringLiteral("true::new text"));

  w.m_col = 0;
  w.renameColumn(QStringLiteral("col name"));
  QCOMPARE(tw->horizontalHeaderItem(0)->text(), QStringLiteral("col name"));

  field->setProperty(QStringLiteral("columns"), QStringLiteral("4"));
  w.updateField(field, field);
  QCOMPARE(tw->columnCount(), 4);
  QCOMPARE(w.text(), QStringLiteral("true::new text"));
  QCOMPARE(spy.count(), 3);

  w.clear();
  QVERIFY(w.text().isEmpty());
}

void FieldWidgetTest::testUrl() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("url"),
                                                         QStringLiteral("url"),
                                                         Tellico::Data::Field::URL));
  Tellico::GUI::URLFieldWidget w(field, nullptr);
  QSignalSpy spy(&w, &Tellico::GUI::FieldWidget::valueChanged);
  QVERIFY(w.expands());

  auto requester = dynamic_cast<KUrlRequester*>(w.widget());
  Q_ASSERT(requester);

  QUrl base = QUrl::fromLocalFile(QFINDTESTDATA("data/relative-link.xml"));
  Tellico::Data::Document::self()->setURL(base); // set the base url
  QUrl link = QUrl::fromLocalFile(QFINDTESTDATA("fieldwidgettest.cpp"));
  requester->setUrl(link);
  QCOMPARE(w.text(), link.url());
  QCOMPARE(spy.count(), 1);

  field->setProperty(QStringLiteral("relative"), QStringLiteral("true"));
  w.updateField(field, field);
  // will be exactly up one level
  QCOMPARE(w.text(), QStringLiteral("../fieldwidgettest.cpp"));

  // check completion
  QCOMPARE(requester->lineEdit()->completionObject()->makeCompletion(QStringLiteral("../fieldwidgettest.c")),
           QStringLiteral("../fieldwidgettest.cpp"));

// verify value after setting the relative link explicitly
  w.setText(QStringLiteral("../fieldwidgettest.cpp"));
  QCOMPARE(w.text(), QStringLiteral("../fieldwidgettest.cpp"));
  QCOMPARE(spy.count(), 1);

  field->setProperty(QStringLiteral("relative"), QStringLiteral("false"));
  w.updateField(field, field);
  // will be exactly up one level
  QCOMPARE(w.text(), link.url());

  w.clear();
  QVERIFY(w.text().isEmpty());
  QVERIFY(requester->url().isEmpty());
}
