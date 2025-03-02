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

#include <config.h>

#include "fieldwidgettest.h"
#include "../gui/boolfieldwidget.h"
#include "../gui/choicefieldwidget.h"
#include "../gui/datefieldwidget.h"
#include "../gui/datewidget.h"
#include "../gui/imagefieldwidget.h"
#include "../gui/imagewidget.h"
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
#include "../images/image.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"

#include <KLocalizedString>
#include <KTextEdit>
#include <KUrlRequester>
#include <KComboBox>
#ifdef HAVE_KSANE
#include <KSaneWidget>
#endif

#include <QTest>
#include <QCheckBox>
#include <QToolButton>
#include <QComboBox>
#include <QTableWidget>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QMimeData>
#include <QLoggingCategory>
#include <QClipboard>

// needs a GUI
QTEST_MAIN( FieldWidgetTest )

void FieldWidgetTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::ImageFactory::init();
  KLocalizedString::setApplicationDomain("tellico");
  QLoggingCategory::setFilterRules(QStringLiteral("tellico.debug = true\ntellico.info = true"));
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

  dw->setDate(QLatin1String("-1-2")); // try an edge case
  QCOMPARE(w.text(), QStringLiteral("-01-02"));

  w.setText(QString());
  QVERIFY(w.text().isEmpty());
  QVERIFY(dw->date().isNull());

  auto spinWidgets = dw->findChildren<Tellico::GUI::SpinBox *>();
  QCOMPARE(spinWidgets.size(), 2);
  auto yearSpin = spinWidgets.at(1);
  auto daySpin = spinWidgets.at(0);
  auto monthCombo = dw->findChild<KComboBox *>();

  monthCombo->setCurrentIndex(1); // index 0 is the empty string
  QCOMPARE(w.text(), QStringLiteral("-01-"));
  QCOMPARE(spy.count(), 2);

  yearSpin->setValue(1900);
  QCOMPARE(w.text(), QStringLiteral("1900-01-"));
  QCOMPARE(spy.count(), 3);

  daySpin->setValue(2);
  QCOMPARE(w.text(), QStringLiteral("1900-01-02"));
  QCOMPARE(spy.count(), 4);
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

  w.setEditEnabled(false); // mimic editing multiple entries
  w.editMultiple(false);
  auto editMultiple = w.m_editMultiple;
  QVERIFY(editMultiple);
  QVERIFY(!le->isEnabled());
  QVERIFY(!editMultiple->isChecked());
  QVERIFY(editMultiple->isHidden());

  w.setEditEnabled(true);
  w.editMultiple(true);
  QVERIFY(le->isEnabled());
  QVERIFY(editMultiple->isChecked());
  QVERIFY(!editMultiple->isHidden());
  QCOMPARE(spy.count(), 3);

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

  sb->stepBy(2);
  QCOMPARE(w.text(), QStringLiteral("5"));
  QCOMPARE(spy.count(), 2);

  // now set AllowMultiple and check that the spinbox is deleted and a line edit is used
  field->setFlags(Tellico::Data::Field::AllowMultiple);
  w.setText(QStringLiteral("1"));
  w.updateField(field, field);
  QVERIFY(!w.isSpinBox());
  auto le = dynamic_cast<QLineEdit*>(w.widget());
  QVERIFY(le);
  // value should be unchanged
  QCOMPARE(w.text(), QStringLiteral("1"));
  QCOMPARE(spy.count(), 2);
  w.setText(QStringLiteral("1;2"));
  QCOMPARE(w.text(), QStringLiteral("1; 2"));
  QCOMPARE(spy.count(), 2);

  le->setText(QStringLiteral("2"));
  QCOMPARE(w.text(), QStringLiteral("2"));
  QCOMPARE(spy.count(), 3);

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

  auto clearButton = rating->findChild<QToolButton *>();
  QVERIFY(clearButton);
  QVERIFY(clearButton->isEnabled());
  clearButton->click();
  QVERIFY(w.text().isEmpty());
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

  auto rowCount = tw->rowCount();
  tw->setCurrentCell(rowCount-1, 0);
  auto item = new QTableWidgetItem(QStringLiteral("last row"));
  tw->setItem(rowCount-1, 0, item);
  QCOMPARE(spy.count(), 4);
  tw->setCurrentCell(rowCount-1, 1);
  // verify a new row is created since the last row was edited
  QCOMPARE(rowCount+1, tw->rowCount());
  delete tw->takeItem(rowCount-1, 0);

  w.m_col = 0;
  w.renameColumn(QStringLiteral("col name"));
  QCOMPARE(tw->horizontalHeaderItem(0)->text(), QStringLiteral("col name"));

  field->setProperty(QStringLiteral("columns"), QStringLiteral("4"));
  w.updateField(field, field);
  QCOMPARE(tw->columnCount(), 4);
  QCOMPARE(w.text(), QStringLiteral("true::new text"));
  QCOMPARE(spy.count(), 4);

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

  w.setText(QString());
  QVERIFY(w.text().isEmpty());
  QVERIFY(requester->url().isEmpty());
}

void FieldWidgetTest::testImage() {
  Tellico::Data::FieldPtr field(new Tellico::Data::Field(QStringLiteral("img"),
                                                         QStringLiteral("img"),
                                                         Tellico::Data::Field::Image));
  Tellico::GUI::ImageFieldWidget w(field, nullptr);
  QSignalSpy spy(&w, &Tellico::GUI::FieldWidget::valueChanged);
  int spyCount = 0;
  QVERIFY(w.expands());
  QVERIFY(w.text().isEmpty());
  auto imgWidget = dynamic_cast<Tellico::GUI::ImageWidget*>(w.widget());
  QVERIFY(imgWidget);
  auto linkOnlyCb = imgWidget->findChild<QCheckBox *>();
  QVERIFY(linkOnlyCb);
  QVERIFY(!linkOnlyCb->isChecked());
  QVERIFY(linkOnlyCb->isEnabled());
  auto editButton = imgWidget->findChild<QToolButton *>();
  QVERIFY(editButton);
  QVERIFY(!editButton->isEnabled());

  field->setProperty(QStringLiteral("link"), QStringLiteral("true"));
  w.updateField(field, field);
  QVERIFY(linkOnlyCb->isChecked());
  linkOnlyCb->setChecked(false);
  QVERIFY(!linkOnlyCb->isChecked());
  QVERIFY(!editButton->isEnabled());

  QUrl u = QUrl::fromLocalFile(QFINDTESTDATA("../../icons/128-apps-tellico.png"));
  // addImage(url, quiet, referer, link)
  QString id = Tellico::ImageFactory::addImage(u, false, QUrl(), true);
  QCOMPARE(id, u.url());
  w.setText(id);
  QCOMPARE(w.text(), id);
  // it's a link-only image so linkOnlyCb should be checked now
  QVERIFY(linkOnlyCb->isChecked());
  QVERIFY(linkOnlyCb->isEnabled());
  QVERIFY(editButton->isEnabled());

  auto img = Tellico::ImageFactory::self()->imageById(id);
  imgWidget->copyImage();
  QCOMPARE(img, QApplication::clipboard()->image(QClipboard::Clipboard));
  QCOMPARE(img, QApplication::clipboard()->image(QClipboard::Selection));

  // unlink the image
  linkOnlyCb->click();
  QVERIFY(!linkOnlyCb->isChecked());
  id = w.text();
  QVERIFY(u.url() != id);
  QVERIFY(!id.isEmpty());
  QCOMPARE(spy.count(), ++spyCount);

  w.clear();
  QVERIFY(w.text().isEmpty());
  QVERIFY(!linkOnlyCb->isChecked());
  QVERIFY(linkOnlyCb->isEnabled());
  QVERIFY(!editButton->isEnabled());

  // different image than before
  imgWidget->m_img = QFINDTESTDATA("../../icons/64-apps-tellico.png");
  imgWidget->slotFinished(); // finished editing the image, now load
  const auto id64 = w.text();
  QVERIFY(!linkOnlyCb->isChecked());
  QVERIFY(!linkOnlyCb->isEnabled());
  QVERIFY(editButton->isEnabled());
  QCOMPARE(spy.count(), ++spyCount);

  QImage img32(QFINDTESTDATA("../../icons/32-apps-tellico.png"));
  QVERIFY(!img32.isNull());
  QPixmap pix = QPixmap::fromImage(img32);
  QVERIFY(!pix.isNull());

  // test a drop event for an image
  auto mimeData = new QMimeData;
  mimeData->setImageData(pix);
  QDragEnterEvent event1(QPoint(), Qt::DropAction::CopyAction, mimeData,
                         Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);
  QVERIFY(qApp->sendEvent(imgWidget, &event1));
  QDropEvent event2(QPoint(), Qt::DropAction::CopyAction, mimeData,
                    Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);
  QVERIFY(qApp->sendEvent(imgWidget, &event2));
  QCOMPARE(spy.count(), ++spyCount);
  QCOMPARE(w.text(), QLatin1String("a1e8417ef84d1ce7f37620810f125093.png"));
  QCOMPARE(pix, imgWidget->m_pixmap);

  w.clear();

  // test a drop event for text
  mimeData = new QMimeData;
  mimeData->setText(QFINDTESTDATA("../../icons/64-apps-tellico.png"));
  QDragEnterEvent event3(QPoint(), Qt::DropAction::CopyAction, mimeData,
                         Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);
  QVERIFY(qApp->sendEvent(imgWidget, &event3));
  QDropEvent event4(QPoint(), Qt::DropAction::CopyAction, mimeData,
                    Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);
  QVERIFY(qApp->sendEvent(imgWidget, &event4));
  QCOMPARE(spy.count(), ++spyCount);
  QCOMPARE(w.text(), id64);

  QVERIFY(!linkOnlyCb->isChecked());
  QVERIFY(linkOnlyCb->isEnabled()); // image can now be linked
  QVERIFY(editButton->isEnabled());

  w.setText(QString());
  // confirm clear
  QVERIFY(w.text().isEmpty());
  QVERIFY(imgWidget->m_pixmap.isNull());
  QVERIFY(imgWidget->m_scaled.isNull());
  QVERIFY(imgWidget->m_originalURL.isEmpty());

#ifdef HAVE_KSANE
  imgWidget->imageReady(img32); // nothing happens since there's no scan widget
  QVERIFY(!linkOnlyCb->isChecked());
  QCOMPARE(spy.count(), spyCount); // no increment

  imgWidget->m_saneWidget = new KSaneIface::KSaneWidget(imgWidget);
  imgWidget->imageReady(img32);
  QVERIFY(!linkOnlyCb->isChecked());
  QVERIFY(!linkOnlyCb->isEnabled());
  QVERIFY(editButton->isEnabled());
  QCOMPARE(spy.count(), ++spyCount);

  // try to unlink the image, which shouldn't work
  linkOnlyCb->click();
  QVERIFY(!linkOnlyCb->isChecked());
  QCOMPARE(w.text(), QLatin1String("dde5bf2cbd90fad8635a26dfb362e0ff.png"));
#endif
}
